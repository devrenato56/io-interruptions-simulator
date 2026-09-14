/*
 * main_gui.c — Simulador interactivo de interrupciones de E/S (Raylib).
 *
 * Interfaz gráfica nativa, en tiempo real, sobre el mismo motor (sim_tick) que
 * ya está verificado. Es el equivalente local del simulador web: escena de
 * hardware (CPU + driver, PIC, controladores/dispositivos), osciloscopio
 * CLK/IRQ/INTA/EOI/DATA, flujo de la Figura 1.4 con nodos que se iluminan,
 * bitácora, IVT, métricas y controles (Reproducir/Paso/Reiniciar + toggles).
 *
 * Compilar:  make gui        (requiere raylib)
 * Ejecutar:  ./simulador_gui
 */
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "simulador.h"

/* ------------------------- paleta (tema oscuro del web) ------------------------- */
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

static Color COLP[3] = {{34,211,238,255},{58,217,160,255},{196,166,255,255}};

/* ------------------------- osciloscopio (buffer) ------------------------- */
#define SCOPE_N 400
static int   sig_clk[SCOPE_N], sig_irq[SCOPE_N], sig_inta[SCOPE_N], sig_eoi[SCOPE_N];
static int   sig_data[SCOPE_N];
static int   sig_len = 0, prev_eoi = 0;

static void scope_sample(const Simulador *S) {
    int clk = S->ciclo % 2;
    int irq = (S->pic.irr[DEV_TIMER] || S->pic.irr[DEV_DISCO] || S->pic.irr[DEV_TECLADO]);
    int inta = (S->in_isr && S->stage == 2);
    int eoi = (S->eoi != prev_eoi); prev_eoi = S->eoi;
    int data = (S->in_isr && S->stage >= 4) ? S->intr_vec : (sig_len ? sig_data[sig_len-1] : 0);
    if (sig_len < SCOPE_N) {
        sig_clk[sig_len]=clk; sig_irq[sig_len]=irq; sig_inta[sig_len]=inta; sig_eoi[sig_len]=eoi; sig_data[sig_len]=data;
        sig_len++;
    } else {
        memmove(sig_clk,sig_clk+1,(SCOPE_N-1)*sizeof(int));
        memmove(sig_irq,sig_irq+1,(SCOPE_N-1)*sizeof(int));
        memmove(sig_inta,sig_inta+1,(SCOPE_N-1)*sizeof(int));
        memmove(sig_eoi,sig_eoi+1,(SCOPE_N-1)*sizeof(int));
        memmove(sig_data,sig_data+1,(SCOPE_N-1)*sizeof(int));
        sig_clk[SCOPE_N-1]=clk; sig_irq[SCOPE_N-1]=irq; sig_inta[SCOPE_N-1]=inta; sig_eoi[SCOPE_N-1]=eoi; sig_data[SCOPE_N-1]=data;
    }
}

/* ------------------------- helpers de dibujo ------------------------- */
static void panel(Rectangle r, const char *titulo) {
    DrawRectangleRounded(r, 0.05f, 8, C_PANEL);
    DrawRectangleRoundedLinesEx(r, 0.05f, 8, 1.0f, C_BORDER);
    if (titulo) DrawText(titulo, (int)r.x+12, (int)r.y+9, 11, C_FAINT);
}
static void chip(Rectangle r, Color borde, float th) {
    DrawRectangleRounded(r, 0.12f, 8, C_PANEL2);
    DrawRectangleRoundedLinesEx(r, 0.12f, 8, th, borde);
}
static void ctext(int cx, int y, const char *t, int size, Color c) {
    DrawText(t, cx - MeasureText(t, size)/2, y, size, c);
}
/* botón: devuelve 1 si se hizo clic */
static int boton(Rectangle r, const char *txt, int activo) {
    Vector2 m = GetMousePosition();
    int hover = CheckCollisionPointRec(m, r);
    Color bg = activo ? C_ACCENT : (hover ? C_BORDER : C_PANEL);
    Color fg = activo ? C_DARKTXT : C_INK;
    DrawRectangleRounded(r, 0.25f, 8, bg);
    DrawRectangleRoundedLinesEx(r, 0.25f, 8, 1.0f, activo ? C_ACCENT : C_BORDER);
    ctext((int)(r.x+r.width/2), (int)(r.y+r.height/2-6), txt, 13, fg);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
/* toggle tipo checkbox: devuelve 1 si se hizo clic */
static int toggle(Rectangle r, const char *txt, int on) {
    Vector2 m = GetMousePosition();
    int hover = CheckCollisionPointRec(m, r);
    DrawRectangleRounded(r, 0.25f, 8, C_PANEL);
    DrawRectangleRoundedLinesEx(r, 0.25f, 8, 1.0f, hover ? C_ACCENT : C_BORDER);
    Rectangle box = { r.x+8, r.y+r.height/2-7, 14, 14 };
    DrawRectangleRounded(box, 0.3f, 6, on ? C_ACCENT : C_PANEL2);
    DrawRectangleRoundedLinesEx(box, 0.3f, 6, 1.0f, on ? C_ACCENT : C_BORDER);
    if (on) DrawText("x", (int)box.x+3, (int)box.y+1, 12, C_DARKTXT);
    DrawText(txt, (int)r.x+28, (int)r.y+r.height/2-6, 12, C_MUTED);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* ------------------------- posiciones de la escena ------------------------- */
/* coords absolutas dentro de la ventana */
typedef struct { float x, y, w, h; } R;
static R RCPU   = {250,150,190,180};
static R RDRV   = {212,118,175,62};
static R RPIC   = {500,132,112,168};
static R RISR   = {250,352,190,42};
static R RREADY = {28,118,118,250};
static R RDEV[3]; /* Timer, Teclado, Disco */

static void proc_pos(const Simulador *S, int id, float *ox, float *oy) {
    if (S->cpu == id) { *ox = RCPU.x + RCPU.w/2; *oy = RCPU.y + RCPU.h - 34; return; }
    /* ¿en la cola de listos? */
    int slot = 0;
    for (int i = 0; i < S->n_ready; i++) if (S->ready[i] == id) {
        *ox = RREADY.x + RREADY.w/2; *oy = RREADY.y + 34 + slot*64; return;
    } else slot++;
    /* bloqueado/pendiente en un dispositivo */
    int dev = S->proc[id].dispositivo >= 0 ? S->proc[id].dispositivo : S->proc[id].pendiente;
    if (dev == DEV_DISCO)   { *ox = RDEV[2].x + RDEV[2].w - 30; *oy = RDEV[2].y + 30; return; }
    if (dev == DEV_TECLADO) { *ox = RDEV[1].x + RDEV[1].w - 30; *oy = RDEV[1].y + 30; return; }
    *ox = RREADY.x + RREADY.w/2; *oy = RREADY.y + 34;
}

int main(void) {
    const int W = 1360, H = 862;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(W, H, "Simulador de Interrupciones · E/S dirigida por interrupciones (Fig. 1.4)");
    SetTargetFPS(60);

    Simulador S; sim_init(&S);
    scope_sample(&S);

#ifdef HEADLESS_CAPTURE
    for (int k = 0; k < 40; k++) { sim_tick(&S); scope_sample(&S); }
#endif

    int reproduciendo = 0;
    float acc = 0.0f;
    int vel = 5;                 /* 1..10 */
    int arrastrando_vel = 0;

    RDEV[0] = (R){700,108,175,78};   /* Timer */
    RDEV[1] = (R){700,200,175,78};   /* Teclado */
    RDEV[2] = (R){700,292,175,78};   /* Disco */

    /* nodos del flujo Fig 1.4, dentro del panel derecho */
    float fx = 908, fy = 484;        /* origen del área del flujo */
    Rectangle FN[7];
    FN[0] = (Rectangle){fx+8,  fy+18, 190, 40};   /* 1 driver */
    FN[1] = (Rectangle){fx+240,fy+18, 190, 40};   /* 2 controller */
    FN[2] = (Rectangle){fx+240,fy+76, 190, 50};   /* 3 genera IRQ */
    FN[3] = (Rectangle){fx+8,  fy+146,190, 50};   /* 4 CPU recibe */
    FN[4] = (Rectangle){fx+8,  fy+214,190, 44};   /* 5 handler */
    FN[5] = (Rectangle){fx+8,  fy+276,190, 40};   /* 6 CPU reanuda */
    const char *FNT[6] = {
        "1 device driver\ninitiates I/O", "2 initiates I/O", "3 input ready/\ncomplete -> IRQ",
        "4 CPU recibe IRQ\n-> handler", "5 handler procesa\n(IRET)", "6 CPU reanuda"
    };

    const char *ETAPAS_C[7] = {
        "Driver inicia la E/S","CPU -> controller","Controlador ejecuta la E/S",
        "Controlador genera la IRQ","CPU salta al handler","Handler procesa - EOI","CPU reanuda la tarea"
    };

    while (!WindowShouldClose()) {
        /* -------- entrada / lógica -------- */
        float dt = GetFrameTime();
        if (reproduciendo) {
            acc += dt;
            float intervalo = 0.62f - (vel-1)*0.06f;   /* 0.62s (lento) .. 0.08s (rápido) */
            while (acc >= intervalo) { acc -= intervalo; sim_tick(&S); scope_sample(&S); }
        }

        BeginDrawing();
        ClearBackground(C_BG);

        /* ================= HEADER ================= */
        DrawText("SISTEMAS OPERATIVOS - E/S DIRIGIDA POR INTERRUPCIONES", 16, 12, 11, C_ACCENT);
        DrawText("Simulador de Interrupciones", 16, 28, 22, C_INK);
        DrawText("Fig. 1.4 - motor en C", 320, 36, 12, C_FAINT);

        Rectangle bPlay={454,16,120,32}, bStep={582,16,86,32}, bReset={676,16,100,32};
        if (boton(bPlay, reproduciendo?"|| Pausa":"> Reproducir", reproduciendo)) reproduciendo=!reproduciendo;
        if (boton(bStep, ">| Paso", 0)) { reproduciendo=0; sim_tick(&S); scope_sample(&S); }
        if (boton(bReset,"<< Reiniciar",0)) { sim_init(&S); sig_len=0; prev_eoi=0; scope_sample(&S); reproduciendo=0; }

        Rectangle tAn={790,16,92,32}, tEo={890,16,132,32}, tAs={1030,16,132,32};
        if (toggle(tAn,"anidar",S.t_anidar)) S.t_anidar=!S.t_anidar;
        if (toggle(tEo,"EOI temprano",S.t_eoi_temprano)) S.t_eoi_temprano=!S.t_eoi_temprano;
        if (toggle(tAs,"E/S asincrona",S.t_asincrono)) S.t_asincrono=!S.t_asincrono;

        /* slider de velocidad */
        Rectangle sl={1180,28,150,8};
        DrawText("Vel", 1180, 12, 11, C_MUTED);
        DrawRectangleRounded(sl,1,6,C_PANEL2); DrawRectangleRoundedLinesEx(sl,1,6,1,C_BORDER);
        float kx = sl.x + (vel-1)/9.0f*sl.width;
        DrawCircle((int)kx,(int)(sl.y+4),8,C_ACCENT);
        Vector2 m=GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m,(Rectangle){sl.x-8,sl.y-8,sl.width+16,24})) arrastrando_vel=1;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) arrastrando_vel=0;
        if (arrastrando_vel) { float t=(m.x-sl.x)/sl.width; if(t<0)t=0; if(t>1)t=1; vel=1+(int)(t*9+0.5f); }

        /* ================= ESCENA ================= */
        Rectangle pScene={14,58,864,372}; panel(pScene,NULL);
        DrawText("COLA DE LISTOS", (int)RREADY.x, (int)RREADY.y-16, 10, C_FAINT);
        DrawText("CPU", (int)RCPU.x+60, 96, 10, C_FAINT);
        DrawText("PIC - CONTROLADOR DE INTERRUPCIONES", (int)RPIC.x-40, 116, 10, C_FAINT);
        DrawText("I/O CONTROLLERS / DISPOSITIVOS", (int)RDEV[0].x-6, 92, 10, C_FAINT);

        /* cola de listos */
        DrawRectangleRounded((Rectangle){RREADY.x,RREADY.y,RREADY.w,RREADY.h},0.06f,8,C_PANEL2);
        DrawRectangleRoundedLinesEx((Rectangle){RREADY.x,RREADY.y,RREADY.w,RREADY.h},0.06f,8,1,C_BORDER);

        /* buses CPU <-> PIC */
        Color busIrqC  = (S.in_isr && S.stage==1)? C_BUSIRQ : C_WIRE;
        Color busAckC  = (S.in_isr && S.stage==2)? C_BUSACK  : C_WIRE;
        Color busDataC = (S.in_isr && (S.stage==2||S.stage==4))? C_BUSDATA : C_WIRE;
        DrawLineEx((Vector2){RCPU.x+RCPU.w,175},(Vector2){RPIC.x,175},2.4f,busIrqC);
        DrawLineEx((Vector2){RCPU.x+RCPU.w,205},(Vector2){RPIC.x,205},2.4f,busAckC);
        DrawLineEx((Vector2){RCPU.x+RCPU.w,235},(Vector2){RPIC.x,235},2.4f,busDataC);
        /* buses PIC <-> dispositivos */
        for (int d=0; d<3; d++) {
            Color c = (S.pic.irr[d==0?DEV_TIMER:d==1?DEV_TECLADO:DEV_DISCO]) ? C_BUSIRQ : C_WIRE;
            DrawLineEx((Vector2){RPIC.x+RPIC.w,RDEV[d].y+30},(Vector2){RDEV[d].x,RDEV[d].y+30},2.0f,C_BUSDATA);
            DrawLineEx((Vector2){RDEV[d].x,RDEV[d].y+50},(Vector2){RPIC.x+RPIC.w,RDEV[d].y+50},2.0f,c);
        }

        /* CPU */
        chip((Rectangle){RCPU.x,RCPU.y,RCPU.w,RCPU.h}, S.in_isr?C_ACCENT:C_BORDER, S.in_isr?2.4f:1.4f);
        ctext((int)(RCPU.x+RCPU.w/2),(int)RCPU.y+62,"CPU",16,C_INK);
        ctext((int)(RCPU.x+RCPU.w/2),(int)RCPU.y+92, S.cpu>=0?TextFormat("P%d",S.cpu+1):"-",18,C_INK);
        ctext((int)(RCPU.x+RCPU.w/2),(int)RCPU.y+116,TextFormat("PC 0x%X",S.pc),9,C_MUTED);
        ctext((int)(RCPU.x+RCPU.w/2),(int)RCPU.y+130,TextFormat("modo %s",S.modo?"kernel":"usuario"),9,C_MUTED);
        DrawCircle((int)RCPU.x+18,(int)RCPU.y+150,4,S.modo?C_WARN:C_GOOD);
        ctext((int)(RCPU.x+RCPU.w/2),(int)RCPU.y+146,TextFormat("EFLAGS 0x%04X",S.eflags),9,C_MUTED);
        DrawCircle((int)RCPU.x+18,(int)RCPU.y+166,4,S.ifbit?C_GOOD:C_DANGER);
        ctext((int)(RCPU.x+RCPU.w/2),(int)RCPU.y+162,TextFormat("IF=%d %s",S.ifbit,S.ifbit?"(IRQ hab.)":"(IRQ enmasc.)"),9,C_MUTED);

        /* línea de chequeo entre instrucciones */
        DrawLineEx((Vector2){RCPU.x+RCPU.w/2,RCPU.y+RCPU.h},(Vector2){RCPU.x+RCPU.w/2,RISR.y},1.4f, Fade(C_WARN,0.5f));

        /* ISR */
        chip((Rectangle){RISR.x,RISR.y,RISR.w,RISR.h}, (S.in_isr&&S.stage>=5)?C_WARN:C_BORDER, 1.4f);
        ctext((int)(RISR.x+RISR.w/2),(int)RISR.y+15,(S.in_isr&&S.stage>=5)?TextFormat("ISR %s - ejecutando",S.cur_dev>=0?DEV_NOMBRE[S.cur_dev]:""):"ISR - inactivo",10,C_MUTED);

        /* DRIVER encima de la CPU */
        chip((Rectangle){RDRV.x,RDRV.y,RDRV.w,RDRV.h}, C_DRIVER, S.driver_busy>0?2.4f:1.4f);
        ctext((int)(RDRV.x+RDRV.w/2),(int)RDRV.y+8,"DRIVER",13,C_INK);
        ctext((int)(RDRV.x+RDRV.w/2),(int)RDRV.y+26, S.driver_busy>0?TextFormat("activo - %s",S.driver_dev>=0?DEV_NOMBRE[S.driver_dev]:""):"ioctl(read)",9,C_MUTED);
        ctext((int)(RDRV.x+RDRV.w/2),(int)RDRV.y+40, S.driver_busy>0?"-> escribe registros":"-> en espera",9,C_MUTED);

        /* PIC */
        chip((Rectangle){RPIC.x,RPIC.y,RPIC.w,RPIC.h}, (S.pic.irr[0]||S.pic.irr[1]||S.pic.irr[2]||S.pic.n_isr)?C_ACCENT:C_BORDER, 1.6f);
        char mirr[4],mimr[4],misr[4]; sim_mascara(S.pic.irr,mirr); sim_mascara(S.pic.imr,mimr); sim_mascara(S.pic.isr,misr);
        ctext((int)(RPIC.x+RPIC.w/2),(int)RPIC.y+10,"PIC",13,C_INK);
        ctext((int)(RPIC.x+RPIC.w/2),(int)RPIC.y+34,TextFormat("IRR %s",mirr),9,C_MUTED);
        ctext((int)(RPIC.x+RPIC.w/2),(int)RPIC.y+48,TextFormat("IMR %s",mimr),9,C_MUTED);
        ctext((int)(RPIC.x+RPIC.w/2),(int)RPIC.y+62,TextFormat("ISR %s",misr),9,C_MUTED);
        ctext((int)(RPIC.x+RPIC.w/2),(int)RPIC.y+78, S.pic.en_servicio>=0?TextFormat("-> %s",DEV_NOMBRE[S.pic.en_servicio]):"-",10,C_ACCENT);
        ctext((int)(RPIC.x+RPIC.w/2),(int)RPIC.y+94,TextFormat("EOI %d",S.eoi),9,C_MUTED);
        /* leds de líneas IRQ (Timer,Disco,Teclado) */
        int ord[3]={DEV_TIMER,DEV_DISCO,DEV_TECLADO};
        for (int i=0;i<3;i++){ Color c=S.pic.irr[ord[i]]?C_DANGER:(S.pic.isr[ord[i]]?C_WARN:C_WIRE);
            DrawCircle((int)RPIC.x+30+i*26,(int)RPIC.y+120,4,c); }
        ctext((int)(RPIC.x+RPIC.w/2),(int)RPIC.y+132,"lineas IRQ",8,C_FAINT);

        /* dispositivos / controladores */
        const char *devlab[3]={"Timer","Teclado","Disco"};
        int devidx[3]={DEV_TIMER,DEV_TECLADO,DEV_DISCO};
        for (int i=0;i<3;i++){
            R rd=RDEV[i]; int di=devidx[i];
            int activo = (di==DEV_TIMER)? (S.timer<=1) : (S.dev[di].sirviendo>=0);
            chip((Rectangle){rd.x,rd.y,rd.w,rd.h}, activo?C_ACCENT:C_BORDER, activo?2.2f:1.4f);
            DrawText(TextFormat("%s controller",devlab[i]),(int)rd.x+12,(int)rd.y+10,12,C_INK);
            /* barra */
            Rectangle bar={rd.x+12,rd.y+34,rd.w-24,10};
            DrawRectangleRounded(bar,1,6,C_PANEL2);
            float frac=0; const char *st="libre";
            if (di==DEV_TIMER){ frac=1.0f-(float)S.timer/S.quantum; st=TextFormat("quantum %d",S.timer); }
            else if (S.dev[di].sirviendo>=0){ int serv=DEV_SERV[di]; frac=1.0f-(float)S.dev[di].restante/(serv>0?serv:1); st=TextFormat("atiende P%d (%d)",S.dev[di].sirviendo+1,S.dev[di].restante); }
            else if (S.dev[di].n_cola>0){ frac=0.15f; st=TextFormat("cola: %d",S.dev[di].n_cola); }
            if(frac<0)frac=0;
            if(frac>1)frac=1;
            DrawRectangleRounded((Rectangle){bar.x,bar.y,bar.width*frac,bar.height},1,6, di==DEV_TIMER?C_WARN:C_ACCENT);
            DrawText(st,(int)rd.x+12,(int)rd.y+52,9,C_MUTED);
        }

        /* píldoras de procesos */
        for (int id=0; id<3; id++){
            float ox,oy; proc_pos(&S,id,&ox,&oy);
            int blocked = (S.proc[id].estado==PROC_BLOQUEADO);
            DrawCircle((int)ox,(int)oy,15, Fade(COLP[id], blocked?0.55f:1.0f));
            if (S.cpu==id) DrawCircleLines((int)ox,(int)oy,17,C_WARN);
            ctext((int)ox,(int)oy-6,TextFormat("P%d",id+1),12,C_DARKTXT);
        }

        /* ================= OSCILOSCOPIO ================= */
        Rectangle pScope={14,438,864,168};
        panel(pScope,"OSCILOSCOPIO - CLK / IRQ / INTA / EOI / DATA[7:0]");
        {
            const char *nom[5]={"CLK","IRQ","INTA","EOI","DATA[7:0]"};
            int *arr[4]={sig_clk,sig_irq,sig_inta,sig_eoi};
            Color col[5]={C_ACCENT,C_BUSIRQ,C_WARN,C_DRIVER,C_BUSDATA};
            float x0=pScope.x+70, x1=pScope.x+pScope.width-14, top=pScope.y+30;
            float rowh=(pScope.height-42)/5.0f;
            int win=28; int start = sig_len>win? sig_len-win : 0; int n=sig_len-start;
            for (int s=0;s<5;s++){
                float yb=top+s*rowh+rowh-8, yt=top+s*rowh+6;
                DrawText(nom[s],(int)pScope.x+10,(int)((yt+yb)/2-5),9,C_MUTED);
                if (s<4){ /* señales digitales */
                    float py=yb, px=x0;
                    for (int i=0;i<n;i++){ int v=arr[s][start+i]; float x=x0+(float)i/(win)* (x1-x0); float y=v?yt:yb;
                        DrawLineEx((Vector2){px,py},(Vector2){x,py},1.6f,col[s]);
                        DrawLineEx((Vector2){x,py},(Vector2){x,y},1.6f,col[s]); py=y; px=x; }
                    DrawLineEx((Vector2){px,py},(Vector2){x1,py},1.6f,col[s]);
                } else { /* bus DATA */
                    float ym=(yt+yb)/2;
                    DrawLineEx((Vector2){x0,ym},(Vector2){x1,ym},1.6f,col[s]);
                    if(sig_len) DrawText(TextFormat("0x%02X",sig_data[sig_len-1]),(int)x1-46,(int)ym-14,10,col[s]);
                }
            }
        }

        /* ================= MÉTRICAS ================= */
        Rectangle pMet={14,614,864,110}; panel(pMet,"INSTRUMENTACION");
        {
            /* Cada valor va a su PROPIO buffer: no se pueden retener varios
               punteros de TextFormat a la vez (raylib recicla sus buffers). */
            const char *ml[9]={"CICLO","IRQ ATEND.","CAMBIOS CTX","USO CPU","LAT IRQ->ISR",
                               "E/S COMPL.","ANIDAMIENTOS","EOI EMITIDOS","IRR PEND."};
            Color mc[9]={C_ACCENT,C_DANGER,C_WARN,C_GOOD,C_WARN,C_ACCENT,C_INK,C_INK,C_DANGER};
            char mv[9][16];
            snprintf(mv[0],16,"%d",S.ciclo);
            snprintf(mv[1],16,"%d",S.irq);
            snprintf(mv[2],16,"%d",S.ctx);
            snprintf(mv[3],16,"%d%%",S.ciclo?(int)(100L*S.busy/S.ciclo):0);
            snprintf(mv[4],16,"%d c",S.irq?S.lat_sum/S.irq:0);
            snprintf(mv[5],16,"%d",S.es);
            snprintf(mv[6],16,"%d",S.nest);
            snprintf(mv[7],16,"%d",S.eoi);
            snprintf(mv[8],16,"%d",S.pic.irr[0]+S.pic.irr[1]+S.pic.irr[2]);
            float mw=(pMet.width-24-8*3)/9.0f;
            for(int i=0;i<9;i++){ Rectangle mr={pMet.x+12+i*(mw+3),pMet.y+34,mw,66};
                DrawRectangleRounded(mr,0.12f,6,C_PANEL2); DrawRectangleRoundedLinesEx(mr,0.12f,6,1,C_BORDER);
                DrawText(mv[i],(int)mr.x+8,(int)mr.y+10,18,mc[i]);
                DrawText(ml[i],(int)mr.x+8,(int)mr.y+42,8,C_FAINT);
            }
        }

        /* ================= IVT ================= */
        Rectangle pIvt={14,732,864,118}; panel(pIvt,"TABLA DE VECTORES (IVT)  -  clic para enmascarar");
        {
            const char *hd="Vec    Fuente     Prio   ISR            Masc";
            DrawText(hd,(int)pIvt.x+14,(int)pIvt.y+30,11,C_FAINT);
            int ordv[3]={DEV_TIMER,DEV_DISCO,DEV_TECLADO};
            for(int i=0;i<3;i++){ int d=ordv[i]; float ry=pIvt.y+48+i*22;
                Rectangle row={pIvt.x+10,ry,pIvt.width-20,20};
                int hover=CheckCollisionPointRec(GetMousePosition(),row);
                if(hover) DrawRectangleRounded(row,0.2f,4,Fade(C_ACCENT,0.08f));
                if(hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) S.pic.imr[d]=!S.pic.imr[d];
                Color tc = S.pic.imr[d]?C_FAINT:C_INK;
                DrawText(TextFormat("0x%02X",DEV_VEC[d]),(int)pIvt.x+14,(int)ry+3,11,tc);
                DrawText(DEV_NOMBRE[d],(int)pIvt.x+70,(int)ry+3,11,tc);
                DrawText(TextFormat("%d",DEV_PRIO[d]),(int)pIvt.x+150,(int)ry+3,11,tc);
                DrawText(TextFormat("isr_%s()",DEV_NOMBRE[d]),(int)pIvt.x+205,(int)ry+3,11,tc);
                DrawText(S.pic.imr[d]?"[enmasc.]":"[activa]",(int)pIvt.x+340,(int)ry+3,11,S.pic.imr[d]?C_DANGER:C_GOOD);
            }
        }

        /* ================= COLUMNA DERECHA ================= */
        /* estado de dispositivos */
        Rectangle pDev={898,58,448,96}; panel(pDev,"ESTADO DE DISPOSITIVOS");
        {
            const char *nm[3]={"Timer","Teclado","Disco"}; int di[3]={DEV_TIMER,DEV_TECLADO,DEV_DISCO};
            for(int i=0;i<3;i++){ float ry=pDev.y+30+i*20; DrawText(nm[i],(int)pDev.x+14,(int)ry,11,C_MUTED);
                const char *v="libre"; int d=di[i];
                if(d==DEV_TIMER) v=TextFormat("quantum %d",S.timer);
                else if(S.dev[d].sirviendo>=0) v=TextFormat("P%d (%d)",S.dev[d].sirviendo+1,S.dev[d].restante);
                else if(S.dev[d].n_cola>0) v=TextFormat("cola %d",S.dev[d].n_cola);
                DrawText(v,(int)pDev.x+120,(int)ry,11,C_INK);
            }
        }
        /* bitácora */
        Rectangle pLog={898,162,448,300}; panel(pLog,"BITACORA DE EVENTOS");
        {
            int maxlines=15;
            for(int i=0;i<maxlines && i<S.nlog;i++){ float ry=pLog.y+30+i*17;
                Color c = S.logtag[i]==1?C_DANGER:S.logtag[i]==2?C_DRIVER:C_WARN;
                DrawText(TextFormat("t%d",S.logt[i]),(int)pLog.x+12,(int)ry,10,C_FAINT);
                DrawText(S.logtag[i]==1?"IRQ":S.logtag[i]==2?"DRV":"SYS",(int)pLog.x+52,(int)ry,10,c);
                /* la fuente por defecto es ASCII: sustituye cada carácter UTF-8
                   multibyte (→, ·, —) por un guion para que se lea bien */
                char msg[80]; int w2=0; const unsigned char *p=(const unsigned char*)S.logtxt[i];
                while (*p && w2<66) {
                    if (*p < 0x80) { msg[w2++]=*p++; }
                    else { msg[w2++]='-'; p++; while ((*p & 0xC0)==0x80) p++; }
                }
                msg[w2]=0;
                DrawText(msg,(int)pLog.x+90,(int)ry,10,C_MUTED);
            }
        }
        /* flujo Fig 1.4 */
        Rectangle pFlow={898,470,448,384}; panel(pFlow,"FLUJO DEL CICLO E/S (FIG. 1.4)");
        {
            /* flowActive (misma lógica del web) */
            int isrDev = S.in_isr && S.cur_dev>=0 && S.cur_dev!=DEV_TIMER;
            int devIrq = S.pic.irr[DEV_DISCO] || S.pic.irr[DEV_TECLADO];
            int sirv = S.dev[DEV_DISCO].sirviendo>=0 || S.dev[DEV_TECLADO].sirviendo>=0;
            int act[6]={0};
            if(S.driver_busy>0){act[0]=1;act[1]=1;}
            if(sirv) act[2]=1;
            if(devIrq || (isrDev&&S.stage<=2)) act[3]=1;
            if(isrDev&&S.stage>=1&&S.stage<=4) act[4]=1;
            if(isrDev&&S.stage>=5&&S.stage<=7) act[5]=1;
            DrawText("CPU",(int)FN[0].x+70,(int)fy+2,11,C_MUTED);
            DrawText("I/O controller",(int)FN[1].x+40,(int)fy+2,11,C_MUTED);
            Color base[6]={C_PANEL2,C_FIGBLUE,C_FIGGRAY,C_PANEL2,C_FIGBLUE2,C_PANEL2};
            /* flechas */
            DrawLineEx((Vector2){FN[0].x+FN[0].width,FN[0].y+20},(Vector2){FN[1].x,FN[1].y+20},1.6f,C_FAINT);
            DrawLineEx((Vector2){FN[1].x+95,FN[1].y+40},(Vector2){FN[2].x+95,FN[2].y},1.6f,C_FAINT);
            DrawLineEx((Vector2){FN[2].x,FN[2].y+26},(Vector2){FN[3].x+FN[3].width,FN[3].y+26},1.6f,C_FAINT);
            DrawLineEx((Vector2){FN[3].x+95,FN[3].y+FN[3].height},(Vector2){FN[4].x+95,FN[4].y},1.6f,C_FAINT);
            DrawLineEx((Vector2){FN[4].x+95,FN[4].y+FN[4].height},(Vector2){FN[5].x+95,FN[5].y},1.6f,C_FAINT);
            DrawLineEx((Vector2){FN[5].x,FN[5].y+20},(Vector2){FN[0].x-6,FN[5].y+20},1.6f,C_FAINT);
            DrawLineEx((Vector2){FN[0].x-6,FN[5].y+20},(Vector2){FN[0].x-6,FN[0].y+20},1.6f,C_FAINT);
            DrawLineEx((Vector2){FN[0].x-6,FN[0].y+20},(Vector2){FN[0].x,FN[0].y+20},1.6f,C_FAINT);
            for(int i=0;i<6;i++){
                DrawRectangleRounded(FN[i],0.1f,6, base[i]);
                DrawRectangleRoundedLinesEx(FN[i],0.1f,6, act[i]?3.0f:1.2f, act[i]?C_WARN:C_BORDER);
                /* texto (2 líneas) */
                Color tc = (i==1||i==2||i==4)?C_DARKTXT:C_INK;
                const char *t=FNT[i]; char l1[40],l2[40]; const char*nl=strchr(t,'\n');
                if(nl){ snprintf(l1,sizeof l1,"%.*s",(int)(nl-t),t); snprintf(l2,sizeof l2,"%s",nl+1);
                    ctext((int)(FN[i].x+FN[i].width/2),(int)FN[i].y+8,l1,9,tc);
                    ctext((int)(FN[i].x+FN[i].width/2),(int)FN[i].y+22,l2,9,tc);
                } else ctext((int)(FN[i].x+FN[i].width/2),(int)FN[i].y+14,t,9,tc);
            }
            /* badge de paso */
            int paso = isrDev? S.stage : devIrq?4 : sirv?3 : S.driver_busy>0?1 : 0;
            Rectangle bg={pFlow.x+12,pFlow.y+348,pFlow.width-24,28};
            DrawRectangleRounded(bg,0.2f,6,C_PANEL2); DrawRectangleRoundedLinesEx(bg,0.2f,6,1,C_BORDER);
            DrawText(TextFormat("PASO %d / 7",paso),(int)bg.x+10,(int)bg.y+6,11,C_WARN);
            DrawText(paso?ETAPAS_C[paso-1]:"en espera de una interrupcion",(int)bg.x+90,(int)bg.y+8,11,C_INK);
        }

        EndDrawing();

#ifdef HEADLESS_CAPTURE
        { static int fr = 0; fr++;
          if (fr == 4) TakeScreenshot("/tmp/gui1.png");
          if (fr == 5) { S.t_anidar = 1; for (int k=0;k<160;k++){ sim_tick(&S); scope_sample(&S);} }
          if (fr == 9) { TakeScreenshot("/tmp/gui2.png"); break; }
        }
#endif
    }
    CloseWindow();
    return 0;
}
