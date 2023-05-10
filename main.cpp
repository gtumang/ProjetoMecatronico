#include "mbed.h"
#include "pinos.h"
#include "TextLCD.h"


InterruptIn chave_fdc_zup(FDC_ZUP);
InterruptIn chave_fdc_zdwn(FDC_ZDWN);
InterruptIn btn_zup(BTN_ZUP);
InterruptIn btn_zdwn(D13);
InterruptIn emergencia(EMER_2);

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
int pos_z;
volatile int en_x;
volatile int en_y;
volatile int en_zup=1;
volatile int en_zdwn=1;
volatile int en_all=1;
volatile int conta_passo = 0;
float k = 1.5/(360/1.8);

//-----------------------------------------------Funcoes--------------------------------------------------------------//
void pulsos_z(){
    MP_Z = !MP_Z;
    if(!dir_z && MP_Z){
        conta_passo++;
    }else if(dir_z && MP_Z){
        conta_passo--;
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
        dir_z = 0;
    }
}
void movimentar_zdwn(){
    if(en_zdwn && en_all){
        CLOCK_Z.attach(&pulsos_z, vel);
        enable_z.write(0);
        dir_z = 1;
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


void referenciar(){
    en_zup = 1;
    while(en_zup){
        movimentar_zup();
    }
    pos_z = 0;

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

//-------------------------------------------------------Loop Principal-------------------------------------------------------------------//
int main()
{
    btn_zup.fall(&movimentar_zup);
    btn_zup.rise(&parar_zup);
    btn_zdwn.fall(&movimentar_zdwn);
    btn_zdwn.rise(&parar_zdwn);
    chave_fdc_zup.fall(&fdc_zup);
    chave_fdc_zup.rise(&liberar_zup);
    chave_fdc_zdwn.fall(&fdc_zdwn);
    chave_fdc_zdwn.rise(&liberar_zdwn);
    emergencia.fall(&parada_emergencia);
    emergencia.rise(&retomar_emergencia);
    while (true) {

        LCD_IHM.locate(0,0);
        if(!en_all){
            LCD_IHM.printf("EMERGENCIA!       ");
            
            // printf("EMERGENCIA!");
        }else{

        LCD_IHM.printf("Passos: %1.2f mm ",conta_passo*k);
        }
        // LCD_IHM.cls();
        printf("\r%i",conta_passo);
    }

}