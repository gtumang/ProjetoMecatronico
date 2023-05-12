#include "mbed.h"
#include "pinos.h"
#include "TextLCD.h"
#include <ctime>

InterruptIn chave_fdc_zup(FDC_ZUP);
InterruptIn chave_fdc_zdwn(FDC_ZDWN);
InterruptIn btn_zup(BTN_ZUP);
InterruptIn btn_zdwn(BTN_ZDWN);
InterruptIn emergencia(EMER_2);
DigitalIn btn_enter(BTN_ENTER);
DigitalIn btn_cancel(BTN_CANCEL);

DigitalOut MP_Z(MOTOR_Z);
DigitalOut MP_Y(MOTOR_Y);
DigitalOut MP_X(MOTOR_X);
DigitalOut enable_z(EN_Z);
DigitalOut dir_x(DIR_X);
DigitalOut dir_y(DIR_Y);
DigitalOut dir_z(DIR_Z);

TextLCD LCD_IHM(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7, TextLCD::LCD20x4);

Ticker CLOCK_Z;
Ticker CLOCK_Y;
Ticker CLOCK_X;


//----------------------------------------------Variaveis globais------------------------------------------------------------//
float vel = 0.001;
volatile int conta_passo = 0;
volatile bool en_x;
volatile bool en_y;
volatile bool en_zup=1;
volatile bool en_zdwn=1;
volatile bool en_all=1;
volatile bool enter = 0;
volatile bool enter_anterior = 0;
volatile bool cancel = 0;
volatile bool cancel_anterior = 0;
volatile bool referenciado = 0;
int ponto_coleta[3];
int pontos_solta[9][3];
float passo_fuso = 1.8; //mm
float passo_motor = 1.8; //graus
float k = passo_fuso/(360.0/passo_motor); 

//-----------------------------------------------Funcoes--------------------------------------------------------------//
void pulsos_z(){
    MP_Z = !MP_Z;
    if(!dir_z && MP_Z){
        conta_passo--;
    }else if(dir_z && MP_Z){
        conta_passo++;
    }
}

void pulsos_y(){
    MP_Y = !MP_Y;

}
void pulsos_x(){
    MP_X = !MP_X;
}

void movimentar_zup(){
    if(en_zup && en_all){
        CLOCK_Z.attach(&pulsos_z, vel);
        enable_z.write(0);
        dir_z.write(0);
    }
}
void movimentar_zdwn(){
    if(en_zdwn && en_all){
        CLOCK_Z.attach(&pulsos_z, vel);
        enable_z.write(0);
        dir_z.write(1);
    }
}

void parar_zup(){
    CLOCK_Z.detach();
    enable_z.write(1);    
}
void parar_zdwn(){
    CLOCK_Z.detach();
    enable_z.write(1);    
}

void liberar_zup(){
     en_zup = 1;
 }

void liberar_zdwn(){
     en_zdwn = 1;
 }

void fdc_zup(){
    parar_zup();
    en_zup = 0;
}
void fdc_zdwn(){
    parar_zdwn();
    en_zdwn = 0;
}

void parada_emergencia(){
    parar_zup();
    parar_zdwn();
    en_all = 0;
}

void retomar_emergencia(){
    en_all = 1;
}


void referenciar(){
    LCD_IHM.cls();
    CLOCK_Z.attach(&pulsos_z, vel);
    enable_z.write(0);
    dir_z.write(0);
    while(en_zup){
        LCD_IHM.locate(3,0);
        LCD_IHM.printf("Referenciando!");

    }
    conta_passo = 0;
    referenciado = 1;
}

void criar_coleta(){
    bool criando_coleta=true;
    bool checando;
    LCD_IHM.cls();
    wait(0.2);
    while(criando_coleta && en_all){
        LCD_IHM.locate(1,0);
        LCD_IHM.printf("Posicione no ponto");
        LCD_IHM.locate(4,1);
        LCD_IHM.printf("de coleta e clique");
        LCD_IHM.locate(7,2);
        LCD_IHM.printf("enter");
        if(!btn_enter){
            btn_zup.fall(NULL);
            btn_zup.rise(NULL);
            btn_zdwn.fall(NULL);
            btn_zdwn.rise(NULL);
            checando = true;
            LCD_IHM.cls();
            while(checando && en_all){
                LCD_IHM.locate(0,0);
                LCD_IHM.printf("Tem certeza?");
                wait(0.2);
                if(!btn_enter){
                    ponto_coleta[0]=0;
                    ponto_coleta[1]=0;
                    ponto_coleta[2]=conta_passo;
                    checando = false;
                    criando_coleta = false;
                }
                if(!btn_cancel){
                    btn_zup.fall(&movimentar_zup);
                    btn_zup.rise(&parar_zup);
                    btn_zdwn.fall(&movimentar_zdwn);
                    btn_zdwn.rise(&parar_zdwn);
                    checando=false;
                }
            }
        }
    }
    btn_zup.fall(&movimentar_zup);
    btn_zup.rise(&parar_zup);
    btn_zdwn.fall(&movimentar_zdwn);
    btn_zdwn.rise(&parar_zdwn);
}

void movimenta_solta(){
    wait(0.2);
    btn_zup.fall(&movimentar_zup);
    btn_zup.rise(&parar_zup);
    btn_zdwn.fall(&movimentar_zdwn);
    btn_zdwn.rise(&parar_zdwn);
    while(en_all){
            LCD_IHM.locate(0,0);
            LCD_IHM.printf("Posicione no ponto");
            LCD_IHM.locate(2,1);
            LCD_IHM.printf("de solta e clique");
            LCD_IHM.locate(7,2);
            LCD_IHM.printf("enter");
            if(!btn_enter){
                break;
            }
        }
}

bool checa_ponto(){
    LCD_IHM.cls();
    wait(0.2);
    btn_zup.fall(NULL);
    btn_zup.rise(NULL);
    btn_zdwn.fall(NULL);
    btn_zdwn.rise(NULL);
    while(en_all){
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("Tem certeza?");
        if(!btn_enter){
            return true;
        }
        if(!btn_cancel){
            return false;
        }
    }
}

void criar_solta(){
    int i=0;
    while(i<10){
        movimenta_solta();
        if(checa_ponto()){
            pontos_solta[i][0]=0;
            pontos_solta[i][1]=0;
            pontos_solta[i][2]=conta_passo;
            i++;
            LCD_IHM.cls();
            LCD_IHM.locate(0,0);
            LCD_IHM.printf("Ponto %i criado", i);
            wait(1);

        }
    }

    btn_zup.fall(&movimentar_zup);
    btn_zup.rise(&parar_zup);
    btn_zdwn.fall(&movimentar_zdwn);
    btn_zdwn.rise(&parar_zdwn);
}

//-------------------------------------------------------Loop Principal-------------------------------------------------------------------//
int main()
{
    chave_fdc_zup.fall(&fdc_zup);
    chave_fdc_zup.rise(&liberar_zup);
    chave_fdc_zdwn.fall(&fdc_zdwn);
    chave_fdc_zdwn.rise(&liberar_zdwn);
    emergencia.fall(&parada_emergencia);
    emergencia.rise(&retomar_emergencia);

    int estado = 1;
    int i =0;

    while (true) {
        //------emergencia-------//
        if(!en_all){
            LCD_IHM.locate(0,0);
            LCD_IHM.printf("EMERGENCIA!    ");
            
            // printf("EMERGENCIA!");
        }
        //-----maquina de estados--------//
        switch(estado){
            //-------Referenciar-------//
            case 1:{
            while(btn_enter && en_all){
                LCD_IHM.locate(7,0);
                LCD_IHM.printf("Deseja");
                LCD_IHM.locate(4,1);
                LCD_IHM.printf("Referenciar?");
            }
            enter_anterior = enter;
            referenciar();
            estado = 2;
            break;
            }

            //-----criar ponto de coleta--------//
            case 2:{
            bool perguntando = true;
            LCD_IHM.cls();
            while(perguntando && en_all){
                LCD_IHM.locate(2,0);
                LCD_IHM.printf("Deseja criar as     ");
                LCD_IHM.locate(5,1);
                LCD_IHM.printf("posicoes?      ");
                if(!btn_enter){
                    btn_zup.fall(&movimentar_zup);
                    btn_zup.rise(&parar_zup);
                    btn_zdwn.fall(&movimentar_zdwn);
                    btn_zdwn.rise(&parar_zdwn);
                    estado = 3;
                    perguntando = false;
                    
                }
                if(!btn_cancel){
                    perguntando = false;
                    estado = 4;
                }
            }
            LCD_IHM.cls();
            break;
            }

            case 3:{
                criar_coleta();
                estado = 4;
            }
            case 4:{
                criar_solta();
                estado=5;
            }
            case 5:{
                if(i>9){
                    i=0;
                }
                LCD_IHM.locate(0,0);
                LCD_IHM.printf("Coleta       Solta %i ",i);
                LCD_IHM.locate(0,1);
                LCD_IHM.printf("X: %1.2f", ponto_coleta[0]*k);
                LCD_IHM.printf("       X: %1.2f", pontos_solta[i][0]*k);
                LCD_IHM.locate(0,2);
                LCD_IHM.printf("Y: %1.2f", ponto_coleta[1]*k);
                LCD_IHM.printf("       Y: %1.2f", pontos_solta[i][1]*k);
                LCD_IHM.locate(0,3);
                LCD_IHM.printf("Z: %1.2f", ponto_coleta[2]*k);
                LCD_IHM.printf("       Z: %1.2f", pontos_solta[i][2]*k);
                wait(2);
                i++;
            }
            

        }
    }
}