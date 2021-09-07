#include "mbed.h"
#include <stdlib.h>

#define TRUE 1
#define ON 1
#define OFF 0
#define ALLON 15

#define FINALINTERVAL 6
#define BEATINTERVAL 150
#define INTERVAL 40

#define LEDNUM 4
#define BOTNUM 4

#define TIMESTART 1000
#define TIMEMAX 1501
#define TIMEMIN 500

#define GAMEWAIT 0
#define GAME 1
#define GAMEFINISH  2
#define GAMESTART 3



/**
 * @brief _eFlanco estructura para determinar el estado del boton
 * 
 */
typedef enum{
    DOWN, //0
    UP,   //1
    RISING, //2
    FALLING, //3      
 } _eFlanco;

/**
 * @brief _eBit mapa de bits con cantidad de botones
 * 
 */
typedef struct{
    unsigned char result: 1;
    unsigned char gameOn: 1;
    unsigned char b2: 1;
    unsigned char b3: 1;

} _eBit;

/**
 * @brief _ePulsador: estructura para almacenar datos de un boton
 *                  state: estado del boton
 *                  timeDown: tiempo en el que se presiono
 *                  timeDiff: tiempo en el que se solto
 */
typedef struct{
    uint8_t state;
    int timeDown;
    int timeDif;
}_ePulsador;

//Variables Globales
Timer miTimer, beat;
_eBit flag;
_ePulsador botones[BOTNUM];
uint16_t mask[] = {1,2,4,8,ALLON};
uint8_t gameSate = GAMEWAIT;


/**
 * @brief funcion que inicia los botones en 0
 * 
 */
void startMef(uint8_t index);

/**
 * @brief funcion para cancelacion de ruido
 * 
 * @param indice numero de boton a actualizar
 */

void updateMef(uint8_t indice);
/**
 * @brief cambia el estado de el bus de salida leds
 * 
 * @param index indice de la mascara a comparar
 * @param state estado en el que se va a poner el led (ON-prender/OFF-apagar)
 */
void togleLed(uint8_t index, uint8_t state);

//Salidas
DigitalOut BEATLED(PC_13);
BusOut leds(PB_12,PB_13,PB_14,PB_15);

//Entradas
BusIn buttons(PB_9,PB_8,PB_7,PB_6);

int main(){
    uint8_t index,  auxIndex, counter = 0, estado = 0;
    uint16_t lednumAux = 0, ledtimeAux = 0, tiempoRandom = 0, timeGameAux = 0, finalTime = 0;
    int beatTime;
    int tiempoMs = 0;
    
    beat.start();
    miTimer.start();
    beatTime = beat.read_ms();
    //Inicializacion de botones en pull up
    for(index = 0;index < BOTNUM; index++){
        startMef(index);
    }
        
    while(TRUE){
        //Parpadeo de led en PC_13 para ver la continuidad del programa
        if((beat.read_ms() - beatTime) >= BEATINTERVAL){
            beatTime = beat.read_ms();
            BEATLED = !BEATLED;
            
        }

        //Maquina de estados para el juego de topos
        switch(gameSate){
            //Espera la presion de algun boton por mas de un segundo
            case GAMEWAIT:
                if ((miTimer.read_ms()-tiempoMs) >= INTERVAL){
                    tiempoMs=miTimer.read_ms();
                    for(uint8_t indice=0; indice < BOTNUM; indice++){
                        updateMef(indice);
                        if(botones[indice].timeDif >= TIMESTART){
                            srand(miTimer.read_us());
                            gameSate = GAMESTART;
                        }
                    }
                }
                break;
            //Controla que no haya botones presionados para iniciar el juego
            case GAMESTART:
                for(auxIndex = 0; auxIndex < BOTNUM; auxIndex++){
                    updateMef(auxIndex);
                    if(botones[auxIndex].state != UP){
                        break;
                    }     
                }
                if(auxIndex == BOTNUM){
                    gameSate = GAME;
                    leds = ALLON;
                    timeGameAux = miTimer.read_ms();
                    for(index = 0;index < BOTNUM; index++){
                        startMef(index);
                    }                    
                }
                break;
            //Juego 
            case GAME:
                if(leds == OFF){
                        if((miTimer.read_ms() - timeGameAux) > TIMESTART){
                            flag.gameOn = ON;
                        }
                        if(flag.gameOn == ON){
                            lednumAux = rand() % (LEDNUM);
                            ledtimeAux = (rand() % (TIMEMAX)) + TIMEMIN;
                            tiempoRandom=miTimer.read_ms();
                            leds=mask[lednumAux];
                            
                        }                       
                }else{
                    if(leds==ALLON) {
                        if((miTimer.read_ms() - timeGameAux) > TIMESTART){
                            leds=OFF;
                            timeGameAux = miTimer.read_ms();
                            flag.gameOn = OFF;
                        }
                    }

                    for(index = 0;index < BOTNUM; index++){
                        updateMef(index);
                        if(botones[index].state == DOWN){
                            if(index == lednumAux){
                                flag.result = ON;
                                gameSate = GAMEFINISH;
                            }else{
                                flag.result = OFF;
                                gameSate = GAMEFINISH;                                                        
                            }

                        }

                    }
                    

                    if (((miTimer.read_ms()-tiempoRandom) > ledtimeAux && flag.gameOn == ON) ){
                        flag.result = OFF;
                        gameSate = GAMEFINISH;
                    } 

                    if(gameSate == GAMEFINISH){
                        leds = OFF;
                        finalTime = miTimer.read_ms();
                    }
                }
                break;
            //Destellos de led dependiendo de si gana o pierde
            case GAMEFINISH:
                
                if(flag.result == OFF){
                    //Pierde
                    if((miTimer.read_ms() - finalTime) > TIMEMIN){
                        estado = !estado;
                        togleLed(lednumAux, estado);
                        counter++;
                        finalTime = miTimer.read_ms();
                    }                    
                }else{
                    //Gana
                    if((miTimer.read_ms() - finalTime) > TIMEMIN){
                        estado = !estado;
                        togleLed(LEDNUM,estado);
                        counter++;
                        finalTime = miTimer.read_ms();
                    }
                }
                //Reinicio de los botones y las variables necesarias para volver a jugar
                if(counter == FINALINTERVAL){
                    leds = OFF;
                    counter = 0;
                    gameSate = GAMEWAIT;
                    tiempoMs = miTimer.read_ms();
                    for(index = 0;index < BOTNUM ;index++){
                        botones[index].timeDif = 0;
                        botones[index].timeDown = 0;
                        startMef(index);
                    }
                    flag.gameOn = OFF;
                }

                break;
            default:
                gameSate = GAMEWAIT;
        }        

    }
    return 0;
}

void startMef(uint8_t index){
    botones[index].state = UP;
}

void updateMef(uint8_t indice){

    switch(botones[indice].state){
        case UP:            
            if(!(buttons.read() & mask[indice]) ){
                botones[indice].state = FALLING;
            }
            break;
            
        case DOWN:
        
            if(buttons.read() & mask[indice] ){
                botones[indice].state = RISING;

                
            }
            break;
        
        case RISING:
            if(buttons.read() & mask[indice] ){
                botones[indice].state = UP;
                botones[indice].timeDif = miTimer.read_ms() - botones[indice].timeDown;
                
                
            }else{
                botones[indice].state = DOWN;
 
            }
            break;
        case FALLING:
            if(!(buttons.read() & mask[indice])){
                botones[indice].state = DOWN;
                botones[indice].timeDown=miTimer.read_ms();
    
            }else{
                botones[indice].state = UP;
            }
            break;
        default:
            startMef(indice);
            break;

    }

}

void togleLed(uint8_t index,uint8_t state){
    switch (state){
        case ON:
            leds = mask[index];
            break;
        case OFF:
            leds = OFF;
        default:
            break;
    }
}

