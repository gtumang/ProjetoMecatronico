#include "TextLCD.h"
#include "mbed.h"
#include "pinos.h"
#include <ctime>

//----------------------------------------------Variaveis
//globais------------------------------------------------------------//
float vel = 0.001;

volatile bool en_all = 1;

Timer t;

volatile bool enter = 0;
volatile bool enter_anterior = 0;
// volatile bool cancel = 0;
// volatile bool cancel_anterior = 0;

uint8_t referenciando = 1;
uint8_t ref_eixo_z = 0;
uint8_t ref_eixo_y = 0;
uint8_t ref_eixo_x = 0;

float passo_fuso = 1.8; //mm
float passo_motor = 1.8; //graus
float k = passo_fuso/(360.0/passo_motor);

enum Estados{
    INICIANDO,
    CHECA_REF,
    REFERENCIAR,
    MENU,
    TELA_COLETA,
    EDITA_COLETA,
    TELA_SOLTA,
    PONTO_SOLTA,
    EDITA_SOLTA,
    QNTS_ML,
    PIPETAGEM,
    PIPETANDO_COLETA,
    PIPETANDO_SOLTA
};

Estados estado, estado_anterior;

volatile uint8_t ml = 0;
uint8_t mls_movidos = 0;

struct Pontos{
    int x;
    int y;
    int z;
    int ml;
};
Pontos ponto_coleta;
Pontos pontos_solta[9];

////--MENU--////
uint8_t linha_cursor;
uint8_t coluna_cursor; //Linha do LCD em que o cursor está atualmente (vai de 1-3)
uint8_t max_cursor = 5; //
uint8_t quantos_pontos = 1; //quantos pontos foram selecionados pelo usuario (vai de 1-9)
uint8_t ponto_add = 5;
uint8_t ponto_escolhido;

//--Pipetagem--//
bool ciclo;
bool subindo_z;
bool movimenta;
bool checa_movimento;
bool descendo_z;
bool checa_ciclo;
uint8_t iterador;
int dx, dy, dz;


//-----Configurando IHM---------//
TextLCD LCD_IHM(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7, TextLCD::LCD20x4);
InterruptIn btn_enter(BTN_ENTER);
DigitalIn btn_cancel(BTN_CANCEL);
InterruptIn btn_emergencia(EMER_2);
DigitalOut pipeta(PIPETA);

//-----Configurando Eixo Y-----//
InterruptIn chave_fdc_yup(FDC_YUP);
InterruptIn chave_fdc_ydwn(FDC_YDWN);

InterruptIn btn_yup(BTN_YUP);
InterruptIn btn_ydwn(BTN_YDWN);

DigitalOut MP_Y(MOTOR_Y);
DigitalOut enable_y(EN_Y);
DigitalOut dir_y(DIR_Y);

Ticker CLOCK_Y;
volatile bool en_ydwn = 1;
volatile bool en_yup = 1;
volatile int conta_passo_y;

void pulsos_y() {    
    MP_Y = !MP_Y;
    if(!dir_y && MP_Y){
        conta_passo_y--;
    }else if(dir_y && MP_Y){
        conta_passo_y++;
    }
}

void movimentar_yup() {
  if (en_yup && en_all) {
    CLOCK_Y.attach(&pulsos_y, vel);
    enable_y.write(0);
    dir_y.write(0);
  }
}
void movimentar_ydwn() {
  if (en_ydwn && en_all) {
    CLOCK_Y.attach(&pulsos_y, vel);
    enable_y.write(0);
    dir_y.write(1);
  }
}

void parar_ydwn() {
  CLOCK_Y.detach();
  enable_y.write(1);
}
void parar_yup() {
  CLOCK_Y.detach();
  enable_y.write(1);
}

void liberar_yup() { en_yup = 1; }

void liberar_ydwn() { en_ydwn = 1; }

void fdc_ydwn() {
  parar_ydwn();
  en_ydwn = 0;
}


void fdc_yup() {
  parar_yup();
  en_yup = 0;
}






//-----Configurando Eixo X-----//
InterruptIn chave_fdc_xup(FDC_XUP);
InterruptIn chave_fdc_xdwn(FDC_XDWN);

InterruptIn btn_xup(BTN_XUP);
InterruptIn btn_xdwn(BTN_XDWN);

DigitalOut MP_X(MOTOR_X);
DigitalOut enable_x(EN_X);
DigitalOut dir_x(DIR_X);

Ticker CLOCK_X;

volatile bool en_xdwn = 1;
volatile bool en_xup = 1;
volatile int conta_passo_x;

void pulsos_x() {
    MP_X = !MP_X;
    if(!dir_x && MP_X){
        conta_passo_x--;
    }else if(dir_x && MP_X){
        conta_passo_x++;
    }
}

void movimentar_xup() {
  if(t.read_ms()>10){   
    if (en_xup && en_all) {
        CLOCK_X.attach(&pulsos_x, vel);
        enable_x.write(0);
        dir_x.write(0);
        t.reset();
    }
  }
}
void movimentar_xdwn() {
  if (en_xdwn && en_all) {
    CLOCK_X.attach(&pulsos_x, vel);
    enable_x.write(0);
    dir_x.write(1);
  }
}

void parar_xdwn() {
  CLOCK_X.detach();
  enable_x.write(1);
}
void parar_xup() {
  CLOCK_X.detach();
  enable_x.write(1);
}

void liberar_xup() { en_xup = 1; }

void liberar_xdwn() { en_xdwn = 1; }

void fdc_xdwn() {
  parar_xdwn();
  en_xdwn = 0;
}


void fdc_xup() {
  parar_xup();
  en_xup = 0;
}


//-----Configurando Eixo Z-----//
InterruptIn chave_fdc_zup(FDC_ZUP);
InterruptIn chave_fdc_zdwn(FDC_ZDWN);

InterruptIn btn_zup(BTN_ZUP);
InterruptIn btn_zdwn(BTN_ZDWN);

DigitalOut MP_Z(MOTOR_Z);
DigitalOut enable_z(EN_Z);
DigitalOut dir_z(DIR_Z);

Ticker CLOCK_Z;

volatile bool en_zdwn = 1;
volatile bool en_zup = 1;
volatile int conta_passo_z;

void pulsos_z() {
    MP_Z = !MP_Z;
    if(!dir_z && MP_Z){
        conta_passo_z--;
    }else if(dir_z && MP_Z){
        conta_passo_z++;
    }
}

void movimentar_zup() {
  if (en_zup && en_all) {
    CLOCK_Z.attach(&pulsos_z, vel);
    enable_z.write(0);
    dir_z.write(0);
  }
}
void movimentar_zdwn() {
  if (en_zdwn && en_all) {
    CLOCK_Z.attach(&pulsos_z, vel);
    enable_z.write(0);
    dir_z.write(1);
  }
}

void parar_zdwn() {
  CLOCK_Z.detach();
  enable_z.write(1);
}
void parar_zup() {
  CLOCK_Z.detach();
  enable_z.write(1);
}

void liberar_zup() { en_zup = 1; }

void liberar_zdwn() { en_zdwn = 1; }

void fdc_zdwn() {
  parar_zdwn();
  en_zdwn = 0;
}


void fdc_zup() {
  parar_zup();
  en_zup = 0;
}

//-----------------------------------------------Funcoes--------------------------------------------------------------//

void habilita_movimento(void){
    btn_zup.fall(&movimentar_zup);
    btn_zup.rise(&parar_zup);
    btn_zdwn.fall(&movimentar_zdwn);
    btn_zdwn.rise(&parar_zdwn);
    btn_yup.fall(&movimentar_yup);
    btn_yup.rise(&parar_yup);
    btn_ydwn.fall(&movimentar_ydwn);
    btn_ydwn.rise(&parar_ydwn);
    btn_xup.fall(&movimentar_xup);
    btn_xup.rise(&parar_xup);
    btn_xdwn.fall(&movimentar_xdwn);
    btn_xdwn.rise(&parar_xdwn);
}

void desabilita_movimento(void){
    btn_zup.fall(NULL);
    btn_zup.rise(NULL);
    btn_zdwn.fall(NULL);
    btn_zdwn.rise(NULL);
    btn_yup.fall(NULL);
    btn_yup.rise(NULL);
    btn_ydwn.fall(NULL);
    btn_ydwn.rise(NULL);
    btn_xup.fall(NULL);
    btn_xup.rise(NULL);
    btn_xdwn.fall(NULL);
    btn_xdwn.rise(NULL);
}

void aumenta_ml(){
    ml++;
}
void diminui_ml(){
    if(ml>0){
        ml--;
    }
}

void emergencia_apertada(void){
    // LCD_IHM.cls();
    en_all=0;
    estado_anterior = estado;
}
void emergencia_solta(void){
    en_all=1;
}

void aperta_enter(void){
    if(t.read_ms()>100){
        enter = !enter;
        t.reset();
    }
}
// void aperta_cancel(void){
//     if(t.read_ms()>100){
//         cancel = !cancel;
//         t.reset();
//     }
// }

void reseta_cursor(void){
    LCD_IHM.locate(0,linha_cursor);
    LCD_IHM.printf(" ");
    LCD_IHM.locate(0,1);
    LCD_IHM.printf(">");
    linha_cursor = 1;
}

void desce_cursor(void){
    if(linha_cursor < 3){        
        LCD_IHM.locate(0,linha_cursor);
        LCD_IHM.printf(" ");
        LCD_IHM.locate(0,linha_cursor+1);
        LCD_IHM.printf(">");
        linha_cursor++;
    }
    
}
void sobe_cursor(void){
    if(linha_cursor>1){
        LCD_IHM.locate(0,linha_cursor);
        LCD_IHM.printf(" ");
        LCD_IHM.locate(0,linha_cursor-1);
        LCD_IHM.printf(">");
        linha_cursor--;
    }
}

void adiciona_ponto(void){
    quantos_pontos++;

}

void reseta_cursor_2(void){
    LCD_IHM.locate(4,3);
    LCD_IHM.printf("^");
    coluna_cursor = 4;
}

void cursor_direita(void){
    if(coluna_cursor < max_cursor){        
        LCD_IHM.locate(coluna_cursor,3);
        LCD_IHM.printf(" ");
        LCD_IHM.locate(coluna_cursor+1,3);
        LCD_IHM.printf("^");
        coluna_cursor++;
    }
    
}
void cursor_esquerda(void){
    if(coluna_cursor>4){
        LCD_IHM.locate(coluna_cursor, 3);
        LCD_IHM.printf(" ");
        LCD_IHM.locate(coluna_cursor-1,3);
        LCD_IHM.printf("^");
        coluna_cursor--; 
    }
}

void atualiza_display(void){
    LCD_IHM.locate(1,2);
    LCD_IHM.printf("                    ");
    LCD_IHM.locate(4,2);
    // LCD_IHM.printf("Teste");
    if(quantos_pontos != 9){
        for(int i=0;i<quantos_pontos;i++){
            LCD_IHM.printf("%i",i+1);
        }
        LCD_IHM.printf("+");
        max_cursor = quantos_pontos + 4;
    }
    else{
        for(int i=0;i<quantos_pontos;i++){
            LCD_IHM.printf("%i",i+1);
        }
        max_cursor = quantos_pontos+3;

    }
}

void ativar_pipeta(void){
    pipeta.write(0);
    wait(2);
    pipeta.write(1);
}

//-----Estados-----//
void iniciando(void){
    LCD_IHM.locate(0,0);
    LCD_IHM.printf("Iniciando maquina!");
    estado = CHECA_REF;
    wait(0.5);
}

void checa_referenciar(void){
    //Iniciando Estado
    if(estado != estado_anterior){
        estado_anterior = estado;
        LCD_IHM.cls();
        LCD_IHM.locate(7,0);
        LCD_IHM.printf("Deseja");
        LCD_IHM.locate(4,1);
        LCD_IHM.printf("Referenciar?");
        wait(0.5);
    }
    if(enter_anterior != enter){
        estado = REFERENCIAR;
        enter_anterior = enter;
    }

    if(estado != estado_anterior){
        
    }
}

void referenciar(void){
    if(estado != estado_anterior){
        estado_anterior = estado;
        LCD_IHM.cls();
        LCD_IHM.locate(3,0);
        LCD_IHM.printf("Referenciando!");
        wait(0.5);
        

    }
    switch(referenciando){
        case 0:{
            estado = MENU;
            LCD_IHM.cls();
            LCD_IHM.locate(0,0);
            LCD_IHM.printf("Referenciamento");
            LCD_IHM.locate(0,1);
            LCD_IHM.printf("completo");
            wait(0.5);
            break;
        }
        case 1:{
            if(!ref_eixo_z){
                movimentar_zup();
                ref_eixo_z = 1;
            }
            if(!en_zup || chave_fdc_zup){
                conta_passo_z = 0;
                referenciando = 2;
            }
            break;
        }
        case 2:{
            if(!ref_eixo_y){
                movimentar_yup();
                ref_eixo_y = 1;
            }
            if(!en_yup || chave_fdc_yup){
                conta_passo_y = 0;
                referenciando = 3;
            }
            break;
        }
        case 3:{
            if(!ref_eixo_x){
                movimentar_xdwn();
                ref_eixo_x = 1;
            }
            if(!en_xdwn || chave_fdc_xdwn){
                conta_passo_x = 0;
                referenciando = 0;
            }
            break;
        }
    }
}



void menu(void){
    if(estado != estado_anterior){
        estado_anterior = estado;
        btn_zup.fall(&sobe_cursor); 
        btn_zdwn.fall(&desce_cursor); 
        btn_xup.fall(NULL);
        btn_xdwn.fall(NULL);
        LCD_IHM.cls();
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("MENU");
        LCD_IHM.locate(1,1);
        LCD_IHM.printf("Ponto Coleta");
        LCD_IHM.locate(1,2);
        LCD_IHM.printf("Pontos Solta");
        LCD_IHM.locate(1,3);
        LCD_IHM.printf("Pipetar");
        linha_cursor = 1;
        LCD_IHM.locate(0,1);
        LCD_IHM.printf(">");
    }
    switch(linha_cursor){
        case 1:{
            if(enter != enter_anterior){
                estado = TELA_COLETA;
                LCD_IHM.cls();
                reseta_cursor();
                enter_anterior = enter;
            }
            break;
        }

        case 2:{
            if(enter != enter_anterior){
                estado = TELA_SOLTA;
                enter_anterior = enter;
                }
            break;
        }

        case 3:{
            if(enter != enter_anterior){
                estado = PIPETAGEM;
                enter_anterior = enter;
                }
            break;
        }
    }
}

void tela_coleta(void);

void edita_coleta(void);

void tela_solta(void);

void ponto_solta(void);

void edita_solta(void);

void qnts_ml(void);

void pipetagem(void);

void pipetando_coleta(void);

void pipetando_solta(void);
//-------------------------------------------------------Loop
//Principal-------------------------------------------------------------------//
int main() {

  wait(1);
//---------Configurações Gerais--------//
  pipeta.write(1);
  t.start();
  btn_emergencia.rise(&emergencia_apertada);
  btn_emergencia.fall(&emergencia_solta);

  btn_enter.fall(&aperta_enter);
//   btn_cancel.fall(&aperta_cancel);

//--Coleta e Solta--//

  ponto_coleta.x = 0;
  ponto_coleta.y = 0;
  ponto_coleta.z = 0;
  ponto_coleta.ml = 0;


// //--------INTERRUPTS EIXO Y----------//

  chave_fdc_ydwn.rise(&fdc_ydwn);
  chave_fdc_ydwn.fall(&liberar_ydwn);

  chave_fdc_yup.rise(&fdc_yup);
  chave_fdc_yup.fall(&liberar_yup);

// //--------INTERRUPTS EIXO X----------//

  chave_fdc_xdwn.rise(&fdc_xdwn);
  chave_fdc_xdwn.fall(&liberar_xdwn);

  chave_fdc_xup.rise(&fdc_xup);
  chave_fdc_xup.fall(&liberar_xup);

//--------INTERRUPTS EIXO Z----------//

  chave_fdc_zdwn.rise(&fdc_zdwn);
  chave_fdc_zdwn.fall(&liberar_zdwn);

  chave_fdc_zup.rise(&fdc_zup);
  chave_fdc_zup.fall(&liberar_zup);


    while (true) {
        
        //-----maquina de estados--------//
        switch(estado){
            //------emergencia-------//
            case INICIANDO:{
                iniciando();
                break;
            }
            case CHECA_REF:{
                checa_referenciar();
                break;
            }
            case REFERENCIAR:{
                referenciar();
                break;
            }
            case MENU:{
                menu();
                break;
            }
            case TELA_SOLTA:{
                tela_solta();
                break;
            }
            case PONTO_SOLTA:{
                ponto_solta();
                break;
            }
            case EDITA_SOLTA:{
                edita_solta();
                break;
            }
            case QNTS_ML:{
                qnts_ml();
                break;
            }
            case TELA_COLETA:{
                tela_coleta();
                break;
            }
            case EDITA_COLETA:{
                edita_coleta();
                break;
            }
            case PIPETAGEM:{
                pipetagem();
                break;
            }
            case PIPETANDO_COLETA:{
                pipetando_coleta();
                break;
            }
            case PIPETANDO_SOLTA:{
                pipetando_solta();
                break;
            }

        }
    }
}


//--Definindo estados--//
void tela_coleta(void){
    if(estado != estado_anterior){
        estado_anterior = estado;
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("Ponto de Coleta");
        LCD_IHM.locate(1,1);
        LCD_IHM.printf("X=%1.0f Y=%1.0f Z=%1.0f ", k*ponto_coleta.x, k*ponto_coleta.y, k*ponto_coleta.z);
        LCD_IHM.locate(1,2);
        LCD_IHM.printf("Editar");
    }

    if(enter_anterior != enter){
        if(linha_cursor == 2){
            estado = EDITA_COLETA;
        }
        enter_anterior = enter;
    }

    if(btn_cancel == 0){
        estado = MENU;
        reseta_cursor();
    }

    if(estado != estado_anterior){

    }
}



void edita_coleta(void){
    int conta_passo_x_anterior = conta_passo_x;
    int conta_passo_y_anterior = conta_passo_y;
    int conta_passo_z_anterior = conta_passo_z;
    if(estado_anterior != estado){
        estado_anterior = estado;
        habilita_movimento();
        LCD_IHM.cls();
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("Editando coleta");
        LCD_IHM.locate(0,1);
        LCD_IHM.printf("X=%1.2f",k*conta_passo_x);
        LCD_IHM.locate(0,2);
        LCD_IHM.printf("Y=%1.2f",k*conta_passo_y);
        LCD_IHM.locate(0,3);
        LCD_IHM.printf("Z=%1.2f",k*conta_passo_z);

    }

    if(conta_passo_x != conta_passo_x_anterior){
        LCD_IHM.locate(2,1);
        LCD_IHM.printf("                 ");
        LCD_IHM.locate(2,1);
        LCD_IHM.printf("%1.2f",k*conta_passo_x);
        conta_passo_x_anterior = conta_passo_x;
    }
    if(conta_passo_y != conta_passo_y_anterior){
        LCD_IHM.locate(2,2);
        LCD_IHM.printf("                 ");
        LCD_IHM.locate(2,2);
        LCD_IHM.printf("%1.2f",k*conta_passo_y);
        conta_passo_y_anterior = conta_passo_y;
    }
    if(conta_passo_z != conta_passo_z_anterior){
        LCD_IHM.locate(2,3);
        LCD_IHM.printf("                 ");
        LCD_IHM.locate(2,3);
        LCD_IHM.printf("%1.2f",k*conta_passo_z);
        conta_passo_z_anterior = conta_passo_z;
    }


    if(enter != enter_anterior){
        ponto_coleta.x = conta_passo_x;
        ponto_coleta.y = conta_passo_y;
        ponto_coleta.z = conta_passo_z;
        estado = MENU;
        enter_anterior = enter;
    }

    if(btn_cancel == 0){
        estado = TELA_COLETA;
    }

    if(estado_anterior != estado){
        desabilita_movimento();
        reseta_cursor();
    }
}

void tela_solta(void){
    if(estado != estado_anterior){
        btn_zup.fall(NULL);
        btn_zdwn.fall(NULL);
        btn_xup.fall(&cursor_direita);
        btn_xdwn.fall(&cursor_esquerda);
        LCD_IHM.cls();
        LCD_IHM.locate(2,0);
        LCD_IHM.printf("Pontos de solta");
        atualiza_display();
        estado_anterior = estado;
        reseta_cursor_2();
    }
    if(enter_anterior != enter){
        if(coluna_cursor == ponto_add){
            adiciona_ponto();
            max_cursor += 1;
            atualiza_display();
            ponto_add++;
        }
        else{
            switch(coluna_cursor){
                case 4:{
                    ponto_escolhido = 1;
                    break;
                }
                case 5:{
                    ponto_escolhido = 2;
                    break;
                }
                case 6:{
                    ponto_escolhido = 3;
                    break;
                }
                case 7:{
                    ponto_escolhido = 4;
                    break;
                }
                case 8:{
                    ponto_escolhido = 5;
                    break;
                }
                case 9:{
                    ponto_escolhido = 6;
                    break;
                }
                case 10:{
                    ponto_escolhido = 7;
                    break;
                }
                case 11:{
                    ponto_escolhido = 8;
                    break;
                }
                case 12:{
                    ponto_escolhido = 9;
                    break;
                }
            }
            estado = PONTO_SOLTA;
        }
        enter_anterior = enter;
    }

    if(btn_cancel == 0){
        estado = MENU;
        reseta_cursor();
    }

    if(estado_anterior != estado){
        btn_zup.fall(&sobe_cursor);
        btn_zdwn.fall(&desce_cursor);
        btn_xup.fall(NULL);
        btn_xdwn.fall(NULL);
    }

}

void ponto_solta(void){
    if(estado != estado_anterior){
        estado_anterior = estado;
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("Ponto de Solta %i", ponto_escolhido);
        LCD_IHM.locate(1,0);
        LCD_IHM.printf("X=%1.0f Y=%1.0f Z=%1.0f ml=%i", k*pontos_solta[ponto_escolhido-1].x, k*pontos_solta[ponto_escolhido-1].y, k*pontos_solta[ponto_escolhido-1].z,pontos_solta[ponto_escolhido-1].ml);
        LCD_IHM.locate(1,2);
        LCD_IHM.printf("Editar");
    }

    if(enter_anterior != enter){
        if(linha_cursor == 2){
            estado = EDITA_SOLTA;
        }
        enter_anterior = enter;
    }

    if(btn_cancel == 0){
        estado = MENU;
        reseta_cursor();
    }

    if(estado != estado_anterior){

    }
}

void edita_solta(void){
    int conta_passo_x_anterior = conta_passo_x;
    int conta_passo_y_anterior = conta_passo_y;
    int conta_passo_z_anterior = conta_passo_z;
    if(estado_anterior != estado){
        estado_anterior = estado;
        habilita_movimento();
        LCD_IHM.cls();
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("Editando Ponto %i", ponto_escolhido);
        LCD_IHM.locate(0,1);
        LCD_IHM.printf("X=%1.2f",k*conta_passo_x);
        LCD_IHM.locate(0,2);
        LCD_IHM.printf("Y=%1.2f",k*conta_passo_y);
        LCD_IHM.locate(0,3);
        LCD_IHM.printf("Z=%1.2f",k*conta_passo_z);

    }

    if(conta_passo_x != conta_passo_x_anterior){
        LCD_IHM.locate(2,1);
        LCD_IHM.printf("                 ");
        LCD_IHM.locate(2,1);
        LCD_IHM.printf("%1.2f",k*conta_passo_x);
        conta_passo_x_anterior = conta_passo_x;
    }
    if(conta_passo_y != conta_passo_y_anterior){
        LCD_IHM.locate(2,2);
        LCD_IHM.printf("                 ");
        LCD_IHM.locate(2,2);
        LCD_IHM.printf("%1.2f",k*conta_passo_y);
        conta_passo_y_anterior = conta_passo_y;
    }
    if(conta_passo_z != conta_passo_z_anterior){
        LCD_IHM.locate(2,3);
        LCD_IHM.printf("                 ");
        LCD_IHM.locate(2,3);
        LCD_IHM.printf("%1.2f",k*conta_passo_z);
        conta_passo_z_anterior = conta_passo_z;
    }


    if(enter != enter_anterior){
        pontos_solta[ponto_escolhido-1].x = conta_passo_x;
        pontos_solta[ponto_escolhido-1].y = conta_passo_y;
        pontos_solta[ponto_escolhido-1].z = conta_passo_z;
        estado = QNTS_ML;
        enter_anterior = enter;
    }

    if(btn_cancel == 0){
        estado = TELA_SOLTA;
    }

    if(estado_anterior != estado){
        btn_zup.fall(NULL);
        btn_zdwn.fall(NULL);
        btn_xup.fall(&cursor_direita);
        btn_xdwn.fall(&cursor_esquerda);
    }
}

void qnts_ml(void){
    if(estado_anterior != estado){
        estado_anterior = estado;
        desabilita_movimento();
        btn_zup.fall(&aumenta_ml);
        btn_zdwn.fall(&diminui_ml);
        LCD_IHM.cls();
        LCD_IHM.locate(3,1);
        LCD_IHM.printf("Quantos ml?: ",ml);
    }

    LCD_IHM.printf("   ");
    LCD_IHM.locate(16,1);
    LCD_IHM.printf("%i",ml);

    if(enter_anterior != enter){
        enter_anterior = enter;
        pontos_solta[ponto_escolhido-1].ml = ml;
        estado  = TELA_SOLTA;
    }

    if(btn_cancel == 0){
        estado = TELA_SOLTA;
    }

    if(estado_anterior != estado){
        desabilita_movimento();
        btn_zup.fall(NULL);
        btn_zdwn.fall(NULL);
        btn_xup.fall(&cursor_direita);
        btn_xdwn.fall(&cursor_esquerda);
    }
}

void pipetagem(void){
    if(estado_anterior != estado){
        if(estado_anterior != PIPETANDO_SOLTA){
            estado_anterior = estado;
            desabilita_movimento();
            LCD_IHM.cls();
            LCD_IHM.locate(0,0);
            LCD_IHM.printf("Iniciando Pipetagem!");
            wait(1);
            ciclo = 1; 
        }
    }   

    if(iterador<quantos_pontos){
        estado = PIPETANDO_COLETA;
    }else{
        iterador = 0;
        ciclo = 0;
        LCD_IHM.cls();
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("Pipetagem Concluida!");
        wait(1);
        estado = MENU;
    }
}

void pipetando_coleta(void){
    if(estado_anterior != estado){
        LCD_IHM.cls();
        LCD_IHM.locate(1,2);
        LCD_IHM.printf("Coletando liquido");
        estado_anterior = estado;
        movimentar_zup();
        subindo_z = 1;
        movimenta = 0;
        checa_movimento = 0;
        descendo_z = 0;
    }
    if(subindo_z){
        if(!en_zup){
            subindo_z = 0;
            movimenta = 1;

        }
    }
    dx = ponto_coleta.x - conta_passo_x;
    dy = ponto_coleta.y - conta_passo_y;
    dz = ponto_coleta.z - conta_passo_z;
    if(movimenta){
        if(dx<0){
            movimentar_xup();
        }
        if(dx>0){
            movimentar_xdwn();
        }
        if(dy<0){
            movimentar_yup();
        }
        if(dy>0){
            movimentar_ydwn();
        }
        movimenta = 0;
        checa_movimento = 1;
    }

    if(checa_movimento){

        if(dx==0 || !en_xup || !en_xdwn){
            parar_xup();
            dx = 0;
        }
        if(dy==0 || !en_yup || !en_ydwn){
            parar_yup();
            dy = 0;
        }
        if(dx==0 & dy==0){
            movimentar_zdwn();
            descendo_z = 1;
            checa_movimento = 0;
        }
    }

    if(descendo_z){
        if(dz == 0){
            parar_zdwn();
            descendo_z = 0;
            ativar_pipeta();
            estado = PIPETANDO_SOLTA;
        }
    }
}

void pipetando_solta(void){
    if(estado_anterior != estado){
        estado_anterior = estado;
        LCD_IHM.cls();
        LCD_IHM.locate(2,1);
        LCD_IHM.printf("Pipetando ponto %i",iterador);
        movimentar_zup();
        subindo_z = 1;
        movimenta = 0;
        checa_movimento = 0;
        descendo_z = 0;
        checa_ciclo = 0;
    }
    if(subindo_z){
        if(!en_zup){
            subindo_z = 0;
            movimenta = 1;
        }
    }
    dx = pontos_solta[iterador].x - conta_passo_x;
    dy = pontos_solta[iterador].y - conta_passo_y;
    dz = pontos_solta[iterador].z - conta_passo_z;
    if(movimenta){
        if(dx<0){
            movimentar_xup();
        }
        if(dx>0){
            movimentar_xdwn();
        }
        if(dy<0){
            movimentar_yup();
        }
        if(dy>0){
            movimentar_ydwn();
        }
        movimenta = 0;
        checa_movimento = 1;
    }

    if(checa_movimento){

        if(dx==0 || !en_xup || !en_xdwn){
            parar_xup();
            dx = 0;
        }
        if(dy==0 || !en_yup || !en_ydwn){
            parar_yup();
            dy = 0;
        }
        if(dx==0 & dy==0){
            movimentar_zdwn();
            descendo_z = 1;
            checa_movimento = 0;
        }
    }

    if(descendo_z){
        if(dz == 0 || !en_zdwn){
            parar_zdwn();
            descendo_z = 0;
            ativar_pipeta();
            checa_ciclo = 1;
            mls_movidos++;
            estado = PIPETAGEM;
        }
    }

    if(mls_movidos >= pontos_solta[iterador].ml){
        iterador++;
        mls_movidos = 0;
    }

    if(estado_anterior != estado){
        LCD_IHM.cls();
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("%i ml transferido", mls_movidos);
        LCD_IHM.locate(0,1);
        LCD_IHM.printf("ao ponto %i", iterador);
        wait(1);
    }
}