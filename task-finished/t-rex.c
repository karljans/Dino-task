//ADC4 - choose the Character
//PC3 - Reset
//PC1 - Show wrongly quessed characters
//PC0 - Enter

#define F_CPU 1000000UL         //1 MHz
#define NR_WORDS 11
#define LCD_NEW_LINE	0xC0 // New LCD line
#define LCD_CLEAR	0x01 // Clear the LCD screen
#define LCD_CARRIAGE_RETURN 0x10 // Go back to begining of line

#include <avr/io.h>                 //Standard include for ATMega16
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h"                        //For driving the LCD
#include <string.h>


/*Function Declarations*/
char charSelection();
ISR(ADC_vect);
int game(char dino);
int obstacleGenerator();
void die();

/*Global Variable Declarations*/
int ADCValue;

/*
 * The main program
 */
int main() {
    char dino;
    int game_status;

    /* Initializations */
    LCD_init();

    // setup port B
    DDRB = 0xFF;
    PORTB = 0x00;
    
    // Setup port C
    DDRC = 0xE4;
    PORTC = 0xFF;

    /* Set up the ADC */
    //AVcc as reference voltage, justify the bits of the analog input to the right and choos ADC4 as the analog input
    ADMUX=(1<<REFS0)|(0<<ADLAR) | (1<<MUX2);

    //Enable ADC, enable interrupts for ADC, prescaler = 128
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);

    //Disable the digital input on ADC4, just to be sure
    DIDR0 = (1 << ADC4D);

    sei();

    ADCSRA |= (1 << ADSC); //
    /* Actual program starts here */
    while(1){
        LCD_command(LCD_CLEAR); // clear lcd
        _delay_ms(5);
        LCD_write("Select Your Dino");
        LCD_command(LCD_NEW_LINE); //Change the line
        LCD_write("Press PC0");

        while((PINC & (1 << PC0)) == (1 << PC0));
        dino = charSelection();
        game_status = game(dino);
        
        if (game_status == 1){
            die();
        }
        
    }

    return 0;
}

/*
 * Select a dino to play with.
 *
 * Return: The chosen dino character
 */
char charSelection() {
    char dino;
    char dino_family[] = {'f', 'q', 0x5C, 0xFC, 0xE6};

    LCD_command(LCD_CLEAR);
    _delay_ms(5);
    LCD_write("Select Your Dino");
    LCD_command(LCD_NEW_LINE); //Change the line

    while((PINC & (1 << PC0)) != (1 << PC0));
    while((PINC & (1 << PC0)) == (1 << PC0)){
        dino = dino_family[ADCValue];
        LCD_putchar(dino);
        LCD_command(LCD_CARRIAGE_RETURN); //Change the line
    }

    return dino;
}

/* The dino game
 *
 * char dino: the dino character we are going to play with
 *
 * return: 1 when we die
 */
int game(char dino) {
    int i;
    
    char temp_buffer;
    char obstacle_buffer[] = "     |    *    o";
    char top_display_buffer[] = "                ";
    char bottom_display_buffer[] = "                ";
    
    while (1){
        LCD_command(LCD_CLEAR);
        _delay_ms(5);
        LCD_write(top_display_buffer);
        _delay_ms(5);
        
        LCD_command(LCD_NEW_LINE);
        _delay_ms(5);
        LCD_write(bottom_display_buffer);
        _delay_ms(500);
                
        //copy obstacles to buffer
        for (i = 0; i < sizeof(bottom_display_buffer); i++){
            bottom_display_buffer[i] = obstacle_buffer[i];
        }
        
        //shift obstacle buffer
        temp_buffer = obstacle_buffer[0];
        for (i = 0; i < sizeof(obstacle_buffer)-1; i++){
            obstacle_buffer[i] = obstacle_buffer[i+1];
        }
        obstacle_buffer[15] = temp_buffer;
        
        //Detect jumping and draw dino
        if ((PINC & (1 << PC0)) != (1 << PC0)){
            top_display_buffer[1] = dino;

        } else {
            top_display_buffer[1] = ' ';
            bottom_display_buffer[1] = dino;
                        
            //obstacle detection
            if (obstacle_buffer[1] != ' '){
                return 1; //We died
            }
        }
    
    }
    return 0;
}

/*
 * Will be executed when the dino dies.
 */
void die(){
    LCD_command(LCD_CLEAR);
    _delay_ms(5);
    LCD_write("R.I.P. Dino! :(");
    LCD_command(LCD_NEW_LINE); //Change the line
    LCD_write("PC3 - Next Life");


    // Dying song
    PORTB |= (1 << PB6) | (1 << PB0);
    _delay_ms(500);
    PORTB &= (0 << PB6) & (1 << PB0);

    while ((PINC & (1 << PC3)) == (1 << PC3));
    return;
}

/*
 *Catch the interrupts made by the ADC and calculate the value of the ADC
 */
//Catch the interrupts of the ADC
ISR(ADC_vect){

    //Let's find the value of the ADC (no need to calculate the voltage)
    ADCValue = (ADCL + (ADCH<<8));

    //Assing a character in the English alphabet to the value of the ADC
    ADCValue = (ADCValue * 5)/1024;
    _delay_us(50);

    //Let's start the conversion again (as it was automatically disabled whet the interrupt fired)
    ADCSRA |= (1 << ADSC);
}
