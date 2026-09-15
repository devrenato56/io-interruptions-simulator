/*
 * main_gui.c — Simulador interactivo de interrupciones de E/S (Raylib).
 *
 * Interfaz gráfica nativa y EN VIVO sobre el mismo motor verificado (sim_tick):
 * cada ciclo se calcula en tiempo real. Incluye:
 *   - Escena de hardware (CPU + driver, PIC, controladores/dispositivos, buses).
 *   - Osciloscopio CLK/IRQ/INTA/EOI/DATA[7:0].
 *   - Flujo de la Figura 1.4 con nodos que se iluminan (flechas reales).
 *   - Bitácora, IVT (clic para enmascarar) y métricas.
 *   - Controles Reproducir/Paso/Reiniciar y toggles anidar/EOI/asíncrona.
 *
 * Lienzo tipo CAD: rueda = zoom al cursor, arrastrar con botón DERECHO = mover,
 * tecla F = ajustar todo a la ventana, tecla R = reiniciar vista. Ventana
 * redimensionable. Cada panel se puede mover (arrastrando su barra de título) y
 * cerrar (X); el botón "Config" los vuelve a mostrar u oculta.
 *
 * Compilar:  make        Ejecutar:  ./simulador_gui
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "rlgl.h"
#include "simulador.h"

/* ------------------------- paleta ------------------------- */
#define C_BG        (Color){  8, 13, 20,255}
#define C_PANEL     (Color){ 15, 26, 36,255}
#define C_PANEL2    (Color){ 11, 20, 29,255}
#define C_BORDER    (Color){ 44, 67, 86,255}
#define C_INK       (Color){233,241,248,255}
#define C_MUTED     (Color){147,169,187,255}
#define C_FAINT     (Color){ 95,118,134,255}
#define C_ACCENT    (Color){ 34,211,238,255}
#define C_WARN      (Color){240,182, 77,255}
#define C_GOOD      (Color){ 58,217,160,255}
#define C_DANGER    (Color){251,111,132,255}
#define C_DRIVER    (Color){240,182, 77,255}
#define C_BUSDATA   (Color){ 99,164,255,255}
#define C_BUSIRQ    (Color){251,111,132,255}
#define C_BUSACK    (Color){240,182, 77,255}
#define C_FIGBLUE   (Color){124,195,232,255}
#define C_FIGGRAY   (Color){200,208,216,255}
#define C_FIGBLUE2  (Color){191,224,242,255}
#define C_DARKTXT   (Color){ 13, 24, 32,255}
#define C_WIRE      (Color){ 49, 72, 92,255}

#define HEADER_H 94
#define WORLD_W  1360.0f
#define WORLD_H  862.0f

/* ------------------------- osciloscopio ------------------------- */
#define SCOPE_N 400
static int sig_clk[SCOPE_N],sig_irq[SCOPE_N],sig_inta[SCOPE_N],sig_eoi[SCOPE_N],sig_data[SCOPE_N];
static int sig_len=0, prev_eoi=0;
static void scope_sample(const Simulador *S){
    int clk=S->ciclo%2;
    int irq=(S->pic.irr[DEV_DISCO]||S->pic.irr[DEV_TECLADO]);
    int inta=(S->in_isr&&S->stage==2);
    int eoi=(S->eoi!=prev_eoi); prev_eoi=S->eoi;
    int data=(S->in_isr&&S->stage>=4)?S->intr_vec:(sig_len?sig_data[sig_len-1]:0);
    if(sig_len<SCOPE_N){ sig_clk[sig_len]=clk;sig_irq[sig_len]=irq;sig_inta[sig_len]=inta;sig_eoi[sig_len]=eoi;sig_data[sig_len]=data;sig_len++; }
    else{ memmove(sig_clk,sig_clk+1,(SCOPE_N-1)*sizeof(int));memmove(sig_irq,sig_irq+1,(SCOPE_N-1)*sizeof(int));
          memmove(sig_inta,sig_inta+1,(SCOPE_N-1)*sizeof(int));memmove(sig_eoi,sig_eoi+1,(SCOPE_N-1)*sizeof(int));
          memmove(sig_data,sig_data+1,(SCOPE_N-1)*sizeof(int));
          sig_clk[SCOPE_N-1]=clk;sig_irq[SCOPE_N-1]=irq;sig_inta[SCOPE_N-1]=inta;sig_eoi[SCOPE_N-1]=eoi;sig_data[SCOPE_N-1]=data; }
}

/* ------------------------- helpers ------------------------- */
static void chip(Rectangle r,Color b,float th){ DrawRectangleRounded(r,0.12f,8,C_PANEL2); DrawRectangleRoundedLinesEx(r,0.12f,8,th,b); }
static void ctext(int cx,int y,const char*t,int s,Color c){ DrawText(t,cx-MeasureText(t,s)/2,y,s,c); }
/* flecha con punta */
static void arrow(Vector2 a,Vector2 b,float th,Color c){
    DrawLineEx(a,b,th,c);
    Vector2 d={b.x-a.x,b.y-a.y}; float L=sqrtf(d.x*d.x+d.y*d.y); if(L<1)return; d.x/=L; d.y/=L;
    Vector2 n={-d.y,d.x}; float s=7.0f;
    Vector2 p1={b.x-d.x*s+n.x*s*0.55f,b.y-d.y*s+n.y*s*0.55f};
    Vector2 p2={b.x-d.x*s-n.x*s*0.55f,b.y-d.y*s-n.y*s*0.55f};
    DrawTriangle(b,p1,p2,c); DrawTriangle(b,p2,p1,c);
}
/* botón en espacio de PANTALLA (header/config) */
static int boton(Rectangle r,const char*txt,int activo){
    Vector2 m=GetMousePosition(); int hover=CheckCollisionPointRec(m,r);
    Color bg=activo?C_ACCENT:(hover?C_BORDER:C_PANEL), fg=activo?C_DARKTXT:C_INK;
    DrawRectangleRounded(r,0.25f,8,bg); DrawRectangleRoundedLinesEx(r,0.25f,8,1,activo?C_ACCENT:C_BORDER);
    ctext((int)(r.x+r.width/2),(int)(r.y+r.height/2-6),txt,13,fg);
    return hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static int toggle(Rectangle r,const char*txt,int on){
    Vector2 m=GetMousePosition(); int hover=CheckCollisionPointRec(m,r);
    DrawRectangleRounded(r,0.25f,8,C_PANEL); DrawRectangleRoundedLinesEx(r,0.25f,8,1,hover?C_ACCENT:C_BORDER);
    Rectangle b={r.x+8,r.y+r.height/2-7,14,14};
    DrawRectangleRounded(b,0.3f,6,on?C_ACCENT:C_PANEL2); DrawRectangleRoundedLinesEx(b,0.3f,6,1,on?C_ACCENT:C_BORDER);
    if(on)DrawText("x",(int)b.x+3,(int)b.y+1,12,C_DARKTXT);
    DrawText(txt,(int)r.x+28,(int)r.y+r.height/2-6,12,C_MUTED);
    return hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* ------------------------- paneles ------------------------- */
enum { P_SCENE, P_SCOPE, P_MET, P_IVT, P_DEV, P_LOG, P_FLOW, NP };
static Rectangle PBASE[NP] = {
    {14,58,864,372},   /* escena */
    {14,438,864,168},  /* osciloscopio */
    {14,614,864,110},  /* métricas */
    {14,732,864,118},  /* IVT */
    {898,58,448,96},   /* dispositivos */
    {898,162,448,300}, /* bitácora */
    {898,470,448,384}, /* flujo */
};
static const char *PNAME[NP]={"Escena","Osciloscopio","Metricas","IVT","Dispositivos","Bitacora","Flujo Fig.1.4"};
static Vector2 poff[NP];
static int pvis[NP];

/* dibuja el marco del panel (barra de título + X) a coords base (dentro del matrix) */
static void panel_frame(int i,const char*titulo){
    Rectangle r=PBASE[i];
    DrawRectangleRounded(r,0.05f,8,C_PANEL);
    DrawRectangleRoundedLinesEx(r,0.05f,8,1,C_BORDER);
    DrawRectangle((int)r.x,(int)r.y,(int)r.width,20,Fade(C_PANEL2,0.9f));
    DrawText(titulo,(int)r.x+12,(int)r.y+6,11,C_FAINT);
    /* X de cierre */
    Rectangle x={r.x+r.width-20,r.y+4,14,14};
    DrawText("x",(int)x.x+3,(int)x.y,13,C_MUTED);
    (void)i;
}

/* ------------------------- posiciones de la escena ------------------------- */
static Rectangle RCPU={250,150,190,180}, RDRV={212,118,175,62}, RPIC={500,132,112,168}, RISR={250,352,190,42};
static Rectangle RDEV[N_DISPOS];

/* ------------------------- dibujo de cada panel (coords base) ------------------------- */
static void draw_scene(Simulador*S){
    panel_frame(P_SCENE,"ESCENA DEL HARDWARE");
    DrawText("SOLICITUDES E/S",28,102,10,C_FAINT);
    for(int d=0;d<N_DISPOS;d++){
        int y=132+d*100;
        DrawText(DEV_NOMBRE[d],28,y,12,d==DEV_DISCO?C_ACCENT:C_GOOD);
        DrawText(TextFormat("En cola: %d",S->dev[d].n_cola),28,y+22,10,C_MUTED);
        DrawText(TextFormat("Espera ISR: %d",S->dev[d].n_pendientes),28,y+40,10,C_MUTED);
        DrawText(TextFormat("Atendidas: %d",S->dev[d].completadas),28,y+58,10,C_MUTED);
    }
    DrawText("CPU",(int)RCPU.x+60,96,10,C_FAINT);
    DrawText("PIC - CONTROLADOR DE INTERRUPCIONES",(int)RPIC.x-40,116,10,C_FAINT);
    DrawText("I/O CONTROLLERS / DISPOSITIVOS",(int)RDEV[0].x-6,92,10,C_FAINT);
    /* buses CPU<->PIC (con flechas) */
    Color cIrq=(S->in_isr&&S->stage==1)?C_BUSIRQ:C_WIRE, cAck=(S->in_isr&&S->stage==2)?C_BUSACK:C_WIRE, cDat=(S->in_isr&&(S->stage==2||S->stage==4))?C_BUSDATA:C_WIRE;
    arrow((Vector2){RPIC.x,175},(Vector2){RCPU.x+RCPU.width,175},2.4f,cIrq);       /* IRQ: PIC->CPU */
    arrow((Vector2){RCPU.x+RCPU.width,205},(Vector2){RPIC.x,205},2.4f,cAck);       /* INTA: CPU->PIC */
    arrow((Vector2){RPIC.x,235},(Vector2){RCPU.x+RCPU.width,235},2.4f,cDat);       /* DATA/vector: PIC->CPU */
    for(int d=0;d<N_DISPOS;d++){
        Color c=S->pic.irr[d]?C_BUSIRQ:C_WIRE;
        /* Las ordenes parten del driver; el PIC solo recibe las IRQ. */
        float by=400+d*12;
        DrawLineEx((Vector2){RDRV.x+RDRV.width,RDRV.y+10+d*12},(Vector2){460+d*12,RDRV.y+10+d*12},1.4f,C_BUSDATA);
        DrawLineEx((Vector2){460+d*12,RDRV.y+10+d*12},(Vector2){460+d*12,by},1.4f,C_BUSDATA);
        DrawLineEx((Vector2){460+d*12,by},(Vector2){RDEV[d].x+RDEV[d].width+4+d*2,by},1.4f,C_BUSDATA);
        DrawLineEx((Vector2){RDEV[d].x+RDEV[d].width+4+d*2,by},(Vector2){RDEV[d].x+RDEV[d].width+4+d*2,RDEV[d].y+30},1.4f,C_BUSDATA);
        arrow((Vector2){RDEV[d].x+RDEV[d].width+4+d*2,RDEV[d].y+30},(Vector2){RDEV[d].x+RDEV[d].width,RDEV[d].y+30},1.4f,C_BUSDATA);
        arrow((Vector2){RDEV[d].x,RDEV[d].y+50},(Vector2){RPIC.x+RPIC.width,RDEV[d].y+50},2.0f,c);         /* IRQ: dev->PIC */
    }
    /* CPU */
    chip(RCPU,S->in_isr?C_ACCENT:C_BORDER,S->in_isr?2.4f:1.4f);
    ctext((int)(RCPU.x+RCPU.width/2),(int)RCPU.y+62,"CPU",16,C_INK);
    ctext((int)(RCPU.x+RCPU.width/2),(int)RCPU.y+92,S->in_isr?"Atendiendo E/S":"Flujo principal",13,C_INK);
    ctext((int)(RCPU.x+RCPU.width/2),(int)RCPU.y+116,TextFormat("PC 0x%X",S->pc),9,C_MUTED);
    ctext((int)(RCPU.x+RCPU.width/2),(int)RCPU.y+130,TextFormat("modo %s",S->modo?"kernel":"usuario"),9,C_MUTED);
    DrawCircle((int)RCPU.x+18,(int)RCPU.y+150,4,S->modo?C_WARN:C_GOOD);
    ctext((int)(RCPU.x+RCPU.width/2),(int)RCPU.y+146,TextFormat("EFLAGS 0x%04X",S->eflags),9,C_MUTED);
    DrawCircle((int)RCPU.x+18,(int)RCPU.y+166,4,S->ifbit?C_GOOD:C_DANGER);
    ctext((int)(RCPU.x+RCPU.width/2),(int)RCPU.y+162,TextFormat("IF=%d %s",S->ifbit,S->ifbit?"(IRQ hab.)":"(IRQ enmasc.)"),9,C_MUTED);
    DrawLineEx((Vector2){RCPU.x+RCPU.width/2,RCPU.y+RCPU.height},(Vector2){RCPU.x+RCPU.width/2,RISR.y},1.4f,Fade(C_WARN,0.5f));
    chip(RISR,(S->in_isr&&S->stage>=4&&S->stage<=6)?C_WARN:C_BORDER,1.4f);
    ctext((int)(RISR.x+RISR.width/2),(int)RISR.y+15,
          S->in_isr?(S->stage==7?"IRET - flujo restaurado":TextFormat("ISR %s",DEV_NOMBRE[S->cur_dev])):"ISR - inactivo",10,C_MUTED);
    /* DRIVER sobre la CPU */
    chip(RDRV,C_DRIVER,S->driver_busy>0?2.4f:1.4f);
    ctext((int)(RDRV.x+RDRV.width/2),(int)RDRV.y+8,"DRIVER",13,C_INK);
    ctext((int)(RDRV.x+RDRV.width/2),(int)RDRV.y+26,S->driver_busy>0?TextFormat("activo - %s",S->driver_dev>=0?DEV_NOMBRE[S->driver_dev]:""):"lectura de E/S",9,C_MUTED);
    ctext((int)(RDRV.x+RDRV.width/2),(int)RDRV.y+40,S->driver_busy>0?"-> escribe registros":"-> en espera",9,C_MUTED);
    /* PIC */
    chip(RPIC,(S->pic.irr[DEV_DISCO]||S->pic.irr[DEV_TECLADO]||S->pic.n_isr)?C_ACCENT:C_BORDER,1.6f);
    char mi[N_DISPOS+1],mm[N_DISPOS+1],ms[N_DISPOS+1]; sim_mascara(S->pic.irr,mi); sim_mascara(S->pic.imr,mm); sim_mascara(S->pic.isr,ms);
    ctext((int)(RPIC.x+RPIC.width/2),(int)RPIC.y+10,"PIC",13,C_INK);
    ctext((int)(RPIC.x+RPIC.width/2),(int)RPIC.y+34,TextFormat("IRR %s",mi),9,C_MUTED);
    ctext((int)(RPIC.x+RPIC.width/2),(int)RPIC.y+48,TextFormat("IMR %s",mm),9,C_MUTED);
    ctext((int)(RPIC.x+RPIC.width/2),(int)RPIC.y+62,TextFormat("ISR %s",ms),9,C_MUTED);
    ctext((int)(RPIC.x+RPIC.width/2),(int)RPIC.y+78,S->pic.en_servicio>=0?TextFormat("-> %s",DEV_NOMBRE[S->pic.en_servicio]):"-",10,C_ACCENT);
    ctext((int)(RPIC.x+RPIC.width/2),(int)RPIC.y+94,TextFormat("EOI %d",S->eoi),9,C_MUTED);
    for(int d=0;d<N_DISPOS;d++){ Color c=S->pic.irr[d]?C_DANGER:(S->pic.isr[d]?C_WARN:C_WIRE); DrawCircle((int)RPIC.x+40+d*26,(int)RPIC.y+120,4,c); }
    ctext((int)(RPIC.x+RPIC.width/2),(int)RPIC.y+132,"lineas IRQ",8,C_FAINT);
    /* dispositivos */
    for(int di=0;di<N_DISPOS;di++){ Rectangle rd=RDEV[di];
        int act=S->dev[di].sirviendo>=0;
        chip(rd,act?C_ACCENT:C_BORDER,act?2.2f:1.4f);
        DrawText(TextFormat("%s controller",DEV_NOMBRE[di]),(int)rd.x+12,(int)rd.y+10,12,C_INK);
        Rectangle bar={rd.x+12,rd.y+34,rd.width-24,10}; DrawRectangleRounded(bar,1,6,C_PANEL2);
        float frac=0; const char*st="libre";
        if(S->dev[di].sirviendo>=0){int sv=DEV_SERV[di];frac=1.0f-(float)S->dev[di].restante/sv;st=TextFormat("E/S #%d (%d)",S->dev[di].sirviendo,S->dev[di].restante);}
        else if(S->dev[di].n_cola>0){frac=0.15f;st=TextFormat("cola: %d",S->dev[di].n_cola);}
        if(frac<0)frac=0;
        if(frac>1)frac=1;
        DrawRectangleRounded((Rectangle){bar.x,bar.y,bar.width*frac,bar.height},1,6,di==DEV_DISCO?C_ACCENT:C_GOOD);
        DrawText(st,(int)rd.x+12,(int)rd.y+52,9,C_MUTED);
    }
}
static void draw_scope(void){
    panel_frame(P_SCOPE,"OSCILOSCOPIO - CLK / IRQ / INTA / EOI / DATA[7:0]");
    Rectangle r=PBASE[P_SCOPE];
    const char*nm[5]={"CLK","IRQ","INTA","EOI","DATA[7:0]"}; int*arr[4]={sig_clk,sig_irq,sig_inta,sig_eoi};
    Color col[5]={C_ACCENT,C_BUSIRQ,C_WARN,C_DRIVER,C_BUSDATA};
    float x0=r.x+70,x1=r.x+r.width-14,top=r.y+30,rowh=(r.height-42)/5.0f; int win=28;
    int start=sig_len>win?sig_len-win:0,n=sig_len-start;
    for(int s=0;s<5;s++){ float yb=top+s*rowh+rowh-8,yt=top+s*rowh+6;
        DrawText(nm[s],(int)r.x+10,(int)((yt+yb)/2-5),9,C_MUTED);
        if(s<4){ float py=yb,px=x0;
            for(int i=0;i<n;i++){int v=arr[s][start+i];float x=x0+(float)i/win*(x1-x0);float y=v?yt:yb;
                DrawLineEx((Vector2){px,py},(Vector2){x,py},1.6f,col[s]); DrawLineEx((Vector2){x,py},(Vector2){x,y},1.6f,col[s]); py=y;px=x;}
            DrawLineEx((Vector2){px,py},(Vector2){x1,py},1.6f,col[s]);
        } else { float ym=(yt+yb)/2; DrawLineEx((Vector2){x0,ym},(Vector2){x1,ym},1.6f,col[s]);
            if(sig_len)DrawText(TextFormat("0x%02X",sig_data[sig_len-1]),(int)x1-46,(int)ym-14,10,col[s]); }
    }
}
static void draw_met(Simulador*S){
    panel_frame(P_MET,"INSTRUMENTACION"); Rectangle r=PBASE[P_MET];
    const char*ml[9]={"CICLO","IRQ ATEND.","CTX ISR","USO CPU","LAT IRQ->ISR","E/S COMPL.","ANIDAMIENTOS","EOI EMITIDOS","IRR PEND."};
    Color mc[9]={C_ACCENT,C_DANGER,C_WARN,C_GOOD,C_WARN,C_ACCENT,C_INK,C_INK,C_DANGER}; char mv[9][16];
    snprintf(mv[0],16,"%d",S->ciclo); snprintf(mv[1],16,"%d",S->irq); snprintf(mv[2],16,"%d",S->contextos_guardados);
    snprintf(mv[3],16,"%d%%",S->ciclo?(int)(100L*S->busy/S->ciclo):0); snprintf(mv[4],16,"%d c",S->irq?S->lat_sum/S->irq:0);
    snprintf(mv[5],16,"%d",S->es); snprintf(mv[6],16,"%d",S->nest); snprintf(mv[7],16,"%d",S->eoi);
    snprintf(mv[8],16,"%d",S->pic.irr_count[DEV_DISCO]+S->pic.irr_count[DEV_TECLADO]);
    float mw=(r.width-24-8*3)/9.0f;
    for(int i=0;i<9;i++){ Rectangle mr={r.x+12+i*(mw+3),r.y+34,mw,66};
        DrawRectangleRounded(mr,0.12f,6,C_PANEL2); DrawRectangleRoundedLinesEx(mr,0.12f,6,1,C_BORDER);
        DrawText(mv[i],(int)mr.x+8,(int)mr.y+10,18,mc[i]); DrawText(ml[i],(int)mr.x+8,(int)mr.y+42,8,C_FAINT); }
}
static void draw_ivt(Simulador*S,Vector2 lm,int can_click){
    panel_frame(P_IVT,"TABLA DE VECTORES (IVT)"); Rectangle r=PBASE[P_IVT];
    DrawText("Vec    Fuente     Prio   ISR            Masc",(int)r.x+14,(int)r.y+30,11,C_FAINT);
    for(int d=0;d<N_DISPOS;d++){ float ry=r.y+48+d*22; Rectangle row={r.x+10,ry,r.width-20,20};
        int hover=CheckCollisionPointRec(lm,row);
        if(hover)DrawRectangleRounded(row,0.2f,4,Fade(C_ACCENT,0.08f));
        if(hover&&can_click&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))S->pic.imr[d]=!S->pic.imr[d];
        Color tc=S->pic.imr[d]?C_FAINT:C_INK;
        DrawText(TextFormat("0x%02X",DEV_VEC[d]),(int)r.x+14,(int)ry+3,11,tc);
        DrawText(DEV_NOMBRE[d],(int)r.x+70,(int)ry+3,11,tc);
        DrawText(TextFormat("%d",DEV_PRIO[d]),(int)r.x+150,(int)ry+3,11,tc);
        DrawText(TextFormat("isr_%s()",DEV_NOMBRE[d]),(int)r.x+205,(int)ry+3,11,tc);
        DrawText(S->pic.imr[d]?"[enmasc.]":"[activa]",(int)r.x+340,(int)ry+3,11,S->pic.imr[d]?C_DANGER:C_GOOD);
    }
}
static void draw_dev(Simulador*S){
    panel_frame(P_DEV,"ESTADO DE DISPOSITIVOS"); Rectangle r=PBASE[P_DEV];
    for(int d=0;d<N_DISPOS;d++){ float ry=r.y+30+d*24; DrawText(DEV_NOMBRE[d],(int)r.x+14,(int)ry,11,C_MUTED);
        const char*v="libre";
        if(S->dev[d].sirviendo>=0)v=TextFormat("E/S #%d (%d)",S->dev[d].sirviendo,S->dev[d].restante);
        else if(S->dev[d].n_cola>0)v=TextFormat("cola %d",S->dev[d].n_cola);
        DrawText(v,(int)r.x+120,(int)ry,11,C_INK);
        DrawText(TextFormat("Espera ISR: %d",S->dev[d].n_pendientes),(int)r.x+270,(int)ry,11,C_MUTED); }
}
static void draw_log(Simulador*S){
    panel_frame(P_LOG,"BITACORA DE EVENTOS"); Rectangle r=PBASE[P_LOG];
    int ml=15;
    for(int i=0;i<ml&&i<S->nlog;i++){ float ry=r.y+30+i*17;
        Color c=S->logtag[i]==1?C_DANGER:S->logtag[i]==2?C_DRIVER:C_WARN;
        DrawText(TextFormat("t%d",S->logt[i]),(int)r.x+12,(int)ry,10,C_FAINT);
        DrawText(S->logtag[i]==1?"IRQ":S->logtag[i]==2?"DRV":"SYS",(int)r.x+52,(int)ry,10,c);
        char msg[80]; int w=0; const unsigned char*p=(const unsigned char*)S->logtxt[i];
        while(*p&&w<66){ if(*p<0x80)msg[w++]=*p++; else{msg[w++]='-';p++;while((*p&0xC0)==0x80)p++;} } msg[w]=0;
        DrawText(msg,(int)r.x+90,(int)ry,10,C_MUTED); }
}
static void draw_flow(Simulador*S){
    panel_frame(P_FLOW,"FLUJO DEL CICLO E/S (FIG. 1.4)"); Rectangle pf=PBASE[P_FLOW];
    float fx=pf.x, fy=pf.y+14;
    Rectangle FN[6]={ {fx+8,fy+18,190,40},{fx+240,fy+18,190,40},{fx+240,fy+76,190,50},
                      {fx+8,fy+146,190,50},{fx+8,fy+214,190,44},{fx+8,fy+276,190,40} };
    const char*T[6]={"1 device driver\ninitiates I/O","2 initiates I/O","3 input ready/\ncomplete -> IRQ",
                     "4 CPU recibe IRQ\n-> handler","5 handler procesa\n(IRET)","6 CPU reanuda"};
    int isrDev=S->in_isr&&S->cur_dev>=0;
    int devIrq=S->pic.irr[DEV_DISCO]||S->pic.irr[DEV_TECLADO];
    int sirv=S->dev[DEV_DISCO].sirviendo>=0||S->dev[DEV_TECLADO].sirviendo>=0;
    int act[6]={0}; if(S->driver_busy>0){act[0]=1;act[1]=1;} if(sirv)act[2]=1;
    if(devIrq||(isrDev&&S->stage<=2))act[3]=1;
    if(isrDev&&S->stage>=3&&S->stage<=6)act[4]=1;
    if(isrDev&&S->stage==7)act[5]=1;
    DrawText("CPU",(int)FN[0].x+70,(int)fy+2,11,C_MUTED); DrawText("I/O controller",(int)FN[1].x+40,(int)fy+2,11,C_MUTED);
    Color base[6]={C_PANEL2,C_FIGBLUE,C_FIGGRAY,C_PANEL2,C_FIGBLUE2,C_PANEL2};
    Color af=C_FAINT;
    arrow((Vector2){FN[0].x+FN[0].width,FN[0].y+20},(Vector2){FN[1].x,FN[1].y+20},1.6f,af);
    arrow((Vector2){FN[1].x+95,FN[1].y+40},(Vector2){FN[2].x+95,FN[2].y},1.6f,af);
    arrow((Vector2){FN[2].x,FN[2].y+26},(Vector2){FN[3].x+FN[3].width,FN[3].y+26},1.6f,af);
    arrow((Vector2){FN[3].x+95,FN[3].y+FN[3].height},(Vector2){FN[4].x+95,FN[4].y},1.6f,af);
    arrow((Vector2){FN[4].x+95,FN[4].y+FN[4].height},(Vector2){FN[5].x+95,FN[5].y},1.6f,af);
    /* realimentación 6->1 (paso 7) */
    DrawLineEx((Vector2){FN[5].x,FN[5].y+20},(Vector2){FN[5].x-8,FN[5].y+20},1.6f,af);
    DrawLineEx((Vector2){FN[5].x-8,FN[5].y+20},(Vector2){FN[0].x-8,FN[0].y+20},1.6f,af);
    arrow((Vector2){FN[0].x-8,FN[0].y+20},(Vector2){FN[0].x,FN[0].y+20},1.6f,af);
    DrawText("7 IRET",(int)FN[0].x-8,(int)((FN[3].y+FN[0].y)/2),8,C_FAINT);
    for(int i=0;i<6;i++){ DrawRectangleRounded(FN[i],0.1f,6,base[i]); DrawRectangleRoundedLinesEx(FN[i],0.1f,6,act[i]?3.0f:1.2f,act[i]?C_WARN:C_BORDER);
        Color tc=(i==1||i==2||i==4)?C_DARKTXT:C_INK; const char*t=T[i]; const char*nl=strchr(t,'\n');
        if(nl){char l1[40],l2[40];snprintf(l1,sizeof l1,"%.*s",(int)(nl-t),t);snprintf(l2,sizeof l2,"%s",nl+1);
            ctext((int)(FN[i].x+FN[i].width/2),(int)FN[i].y+8,l1,9,tc); ctext((int)(FN[i].x+FN[i].width/2),(int)FN[i].y+22,l2,9,tc);}
        else ctext((int)(FN[i].x+FN[i].width/2),(int)FN[i].y+14,t,9,tc); }
    int paso=isrDev?S->stage:devIrq?4:sirv?3:S->driver_busy>0?1:0;
    const char*E[7]={"Driver inicia la E/S","CPU -> controller","Controlador ejecuta la E/S","Controlador genera la IRQ","CPU salta al handler","Handler procesa - EOI","CPU reanuda el flujo"};
    const char*EI[7]={"IRQ aceptada","INTA al PIC","Contexto preservado","Vector -> ISR","Atencion / EOI temprano","Resultado E/S y EOI","IRET: flujo restaurado"};
    Rectangle bg={pf.x+12,pf.y+348,pf.width-24,28}; DrawRectangleRounded(bg,0.2f,6,C_PANEL2); DrawRectangleRoundedLinesEx(bg,0.2f,6,1,C_BORDER);
    DrawText(TextFormat("PASO %d / 7",paso),(int)bg.x+10,(int)bg.y+6,11,C_WARN);
    DrawText(paso?(isrDev?EI[paso-1]:E[paso-1]):"en espera de una interrupcion",(int)bg.x+90,(int)bg.y+8,11,C_INK);
}

int main(void){
    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE);
#ifdef HEADLESS_CAPTURE
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
#endif
    InitWindow(1360,780,"Simulador de Interrupciones - E/S dirigida por interrupciones (Fig. 1.4)");
    SetWindowMinSize(1060,600);
    SetTextureFilter(GetFontDefault().texture,TEXTURE_FILTER_BILINEAR);
    SetTargetFPS(60);
    Simulador S; sim_init(&S); scope_sample(&S);
    for(int i=0;i<NP;i++){poff[i]=(Vector2){0,0};pvis[i]=1;}
    Camera2D cam={0}; cam.zoom=1;
    int fit_pending=1;                 /* ajustar la vista en el primer frame */
    int reproduciendo=0; float acc=0; int vel=5, drag_vel=0;
    int dragging=-1; int show_config=0;

#ifdef HEADLESS_CAPTURE
    for(int k=0;k<40;k++){sim_tick(&S);scope_sample(&S);}
#endif

    while(!WindowShouldClose()){
        float dt=GetFrameTime();
        if(reproduciendo){ acc+=dt; float iv=0.62f-(vel-1)*0.06f; while(acc>=iv){acc-=iv;sim_tick(&S);scope_sample(&S);} }

        /* ---- ajustar vista (F/R o inicio) ---- */
        int fitnow=fit_pending||IsKeyPressed(KEY_F)||IsKeyPressed(KEY_R);
        if(fitnow){ float aw=GetScreenWidth()-16, ah=GetScreenHeight()-HEADER_H-12;
            float z=fminf(aw/WORLD_W,ah/WORLD_H); if(z<0.1f)z=0.1f;
            cam.zoom=z; cam.offset=(Vector2){8,HEADER_H+6}; cam.target=(Vector2){0,0}; fit_pending=0; }

        Vector2 mouse=GetMousePosition();
        int in_canvas = mouse.y>HEADER_H && !(show_config && mouse.x>GetScreenWidth()-250 && mouse.y<420);

        /* ---- zoom / pan del lienzo ---- */
        if(in_canvas){
            float w=GetMouseWheelMove();
            if(w!=0){ Vector2 b=GetScreenToWorld2D(mouse,cam); cam.zoom*=(w>0?1.1f:1/1.1f);
                if(cam.zoom<0.2f)cam.zoom=0.2f;
                if(cam.zoom>3.0f)cam.zoom=3.0f;
                Vector2 a=GetScreenToWorld2D(mouse,cam); cam.target.x+=b.x-a.x; cam.target.y+=b.y-a.y; }
            if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)||IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)){
                Vector2 d=GetMouseDelta(); cam.target.x-=d.x/cam.zoom; cam.target.y-=d.y/cam.zoom; }
        }

        Vector2 wm=GetScreenToWorld2D(mouse,cam);

        /* ---- arrastre / cierre de paneles (botón izquierdo) ---- */
        if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging=-1;
        if(in_canvas && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && dragging<0){
            for(int i=NP-1;i>=0;i--){ if(!pvis[i])continue; Rectangle r=PBASE[i]; Vector2 o=poff[i];
                Rectangle xb={r.x+o.x+r.width-20,r.y+o.y+4,14,14};
                Rectangle tb={r.x+o.x,r.y+o.y,r.width,20};
                if(CheckCollisionPointRec(wm,xb)){ pvis[i]=0; break; }
                if(CheckCollisionPointRec(wm,tb)){ dragging=i; break; }
                if(CheckCollisionPointRec(wm,(Rectangle){r.x+o.x,r.y+o.y,r.width,r.height})) break; /* clic dentro: no atraviesa */
            }
        }
        if(dragging>=0){ Vector2 d=GetMouseDelta(); poff[dragging].x+=d.x/cam.zoom; poff[dragging].y+=d.y/cam.zoom; }

        BeginDrawing();
        ClearBackground(C_BG);

        /* ================= LIENZO (mundo, con zoom/pan) ================= */
        BeginMode2D(cam);
        RDEV[DEV_DISCO]=(Rectangle){680,148,175,78}; RDEV[DEV_TECLADO]=(Rectangle){680,270,175,78};
        for(int i=0;i<NP;i++){ if(!pvis[i])continue;
            rlPushMatrix(); rlTranslatef(poff[i].x,poff[i].y,0);
            int canclick = (dragging<0) && in_canvas;
            switch(i){
                case P_SCENE: draw_scene(&S); break;
                case P_SCOPE: draw_scope(); break;
                case P_MET:   draw_met(&S); break;
                case P_IVT:   draw_ivt(&S,(Vector2){wm.x-poff[i].x,wm.y-poff[i].y},canclick); break;
                case P_DEV:   draw_dev(&S); break;
                case P_LOG:   draw_log(&S); break;
                case P_FLOW:  draw_flow(&S); break;
            }
            rlPopMatrix();
        }
        EndMode2D();

        /* ================= HEADER (pantalla, fijo) ================= */
        DrawRectangle(0,0,GetScreenWidth(),HEADER_H,C_BG);
        DrawLine(0,HEADER_H,GetScreenWidth(),HEADER_H,C_BORDER);
        DrawText("Simulador de Interrupciones",12,8,20,C_INK);
        DrawText("Fig. 1.4 - motor en C",12,32,11,C_FAINT);
        /* estado en vivo */
        DrawCircle(166,38,4,reproduciendo?C_GOOD:C_WARN);
        DrawText(reproduciendo?"EN VIVO":"EN PAUSA",176,32,12, reproduciendo?C_GOOD:C_WARN);

        Rectangle bPlay={12,52,116,32},bStep={134,52,80,32},bReset={220,52,96,32};
        if(boton(bPlay,reproduciendo?"|| Pausa":"> Reproducir",reproduciendo))reproduciendo=!reproduciendo;
        if(boton(bStep,">| Paso",0)){reproduciendo=0;sim_tick(&S);scope_sample(&S);}
        if(boton(bReset,"<< Reiniciar",0)){sim_init(&S);sig_len=0;prev_eoi=0;scope_sample(&S);reproduciendo=0;}
        Rectangle tAn={328,52,86,32},tEo={420,52,126,32},tAs={552,52,126,32};
        if(toggle(tAn,"anidar",S.t_anidar))S.t_anidar=!S.t_anidar;
        if(toggle(tEo,"EOI temprano",S.t_eoi_temprano))S.t_eoi_temprano=!S.t_eoi_temprano;
        if(toggle(tAs,"E/S asincrona",S.t_asincrono))S.t_asincrono=!S.t_asincrono;
        /* velocidad */
        float sx=GetScreenWidth()-330; Rectangle sl={sx,26,110,8};
        DrawText("Vel",(int)sx,10,11,C_MUTED);
        DrawRectangleRounded(sl,1,6,C_PANEL2); DrawRectangleRoundedLinesEx(sl,1,6,1,C_BORDER);
        DrawCircle((int)(sl.x+(vel-1)/9.0f*sl.width),(int)(sl.y+4),8,C_ACCENT);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,(Rectangle){sl.x-8,sl.y-8,sl.width+16,24}))drag_vel=1;
        if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT))drag_vel=0;
        if(drag_vel){float t=(mouse.x-sl.x)/sl.width;if(t<0)t=0;if(t>1)t=1;vel=1+(int)(t*9+0.5f);}
        /* ajustar vista + config */
        Rectangle bFit={GetScreenWidth()-212,10,70,32}, bCfg={GetScreenWidth()-134,10,78,32};
        if(boton(bFit,"Ajustar",0))fit_pending=1;
        if(boton(bCfg,"Config",show_config))show_config=!show_config;

        /* ================= POPUP CONFIG ================= */
        if(show_config){ Rectangle cp={GetScreenWidth()-244,HEADER_H+6,236,300};
            DrawRectangleRounded(cp,0.05f,8,C_PANEL); DrawRectangleRoundedLinesEx(cp,0.05f,8,1,C_ACCENT);
            DrawText("VISTAS",(int)cp.x+14,(int)cp.y+12,11,C_FAINT);
            for(int i=0;i<NP;i++){ Rectangle tr={cp.x+12,cp.y+34+i*28,cp.width-24,24};
                if(toggle(tr,PNAME[i],pvis[i]))pvis[i]=!pvis[i]; }
            Rectangle rb={cp.x+12,cp.y+34+NP*28+4,cp.width-24,28};
            if(boton(rb,"Reiniciar layout",0)){ for(int i=0;i<NP;i++){poff[i]=(Vector2){0,0};pvis[i]=1;} fit_pending=1; }
        }

        EndDrawing();

#ifdef HEADLESS_CAPTURE
        { static int fr=0; fr++;
          if(fr==4) TakeScreenshot("gui1.png");
          if(fr==5){
              S.t_anidar=1;
              for(int k=0;k<160;k++){sim_tick(&S);scope_sample(&S);}
              SetWindowSize(1060,600);
              fit_pending=1;
          }
          if(fr==9){ TakeScreenshot("gui2.png"); break; }
        }
#endif
    }
    CloseWindow();
    return 0;
}
