#include "mbed.h"
#include "pinos.h"
#include "TextLCD.h"
#include <ctime>

InterruptIn chave_fdc_zup(FDC_ZUP);
InterruptIn chave_fdc_zdwn(FDC_ZDWN);
InterruptIn btn_zup(BTN_ZUP);
InterruptIn btn_zdwn(BTN_ZDWN);
InterruptIn emergencia(EMER_2);
InterruptIn btn_enter(BTN_ENTER);
InterruptIn btn_cancel(BTN_CANCEL);

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
volatile short criar_posicao;
float passo_fuso = 1.8; //mm
float passo_motor = 1.8; //graus
float k = passo_fuso/(360.0/passo_motor); 

int posicoes[2][9];

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

void confirma(){
    enter=!enter_anterior;
}
void cancela(){
    cancel=!cancel_anterior;
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
    criar_posicao = 0;
    LCD_IHM.cls();
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
    btn_enter.fall(&confirma);
    btn_cancel.fall(&cancela);

    while (true) {
        if(!en_all){
            LCD_IHM.locate(0,0);
            LCD_IHM.printf("EMERGENCIA!    ");
            
            // printf("EMERGENCIA!");
        }else if(!referenciado){
            while(enter==enter_anterior){
                LCD_IHM.locate(7,0);
                LCD_IHM.printf("Deseja");
                LCD_IHM.locate(4,1);
                LCD_IHM.printf("Referenciar?");
            }
            referenciar();
        }
        else{
        LCD_IHM.locate(0,0);
        LCD_IHM.printf("Z: %1.2f mm    ",conta_passo*k);
        LCD_IHM.locate(0,1);
        LCD_IHM.printf("Passos: %i       ",abs(conta_passo));
        }
        // LCD_IHM.cls();
        printf("\n\r%i",posicoes[0][0]);
        for(int x; x<9; x++){
            printf("\n\r%i",posicoes[1][x]);
        }
    }
    enter_anterior = enter;
}