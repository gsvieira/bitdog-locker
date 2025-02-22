#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#define VRX 27
#define VRY 26
#define BTN_A 5
#define BTN_B 6
#define ADC_CHANNEL_0 0
#define ADC_CHANNEL_1 1
#define SW 22
#define SERVO_CTL 28
#define PWM_DIVIDER 250.0f
#define PERIOD 10000
#define PASS_LENGTH 4


char numbers[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
uint8_t password_att[PASS_LENGTH];
uint16_t vry_num=0;


void setup_joystick()
{
    adc_init();
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);
}

void setup_gpio()
{
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
}

void setup_pwm()
{
    uint slice;
    gpio_set_function(SERVO_CTL, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(SERVO_CTL);
    pwm_set_clkdiv(slice, PWM_DIVIDER);
    pwm_set_wrap(slice, PERIOD);
    pwm_set_enabled(slice, false);
}
void setup_pass()
{
    for(uint8_t i = 0; i<PASS_LENGTH; i++){
        password_att[i] = 0;
    }
}

void setup()
{
    stdio_init_all();
    setup_joystick();
    setup_gpio();
    setup_pass();
    setup_pwm();
}

void read_joystick_value()
{
    adc_select_input(ADC_CHANNEL_1);
    vry_num = adc_read();
}

void number_selector(uint8_t *position)
{
    if(vry_num < ((4096/2)-200)) //verify if goes left
    {
        if(*position == 0){
            *position = 9;
        } else {
            (*position)--;
        }
    } 
    else if(vry_num > ((4096/2)+200)) //verify if goes right
    {
        if(*position == 9){
            *position = 0;
        } else {
            (*position)++;
        }
    }
    printf("New position: %d\n", *position);
}

bool get_password(uint8_t* curr_pos, uint8_t* position)
{
    password_att[*curr_pos] = *position;
    if (*curr_pos == PASS_LENGTH - 1){
        *curr_pos = 0; // reset to accept new attempt
        return 0;
    }
    (*curr_pos)++;
    printf("Get new char!\n");
    return 1;
}

void delete_password(uint8_t* curr_pos)
{
    if(*curr_pos == 0){
        printf("can't delete\n");
        return;
    } else {
        (*curr_pos)--;
        printf("deleted num, curr_pos: %s\n", *curr_pos);
    }

}

void change_password(uint8_t password[])
{
    for(uint8_t i=0; i < PASS_LENGTH; i++) {
        password[i] = password_att[i];
    }
}

bool check_password(uint8_t password[], uint8_t* curr_pos, bool *locker_opened)
{
    for(uint8_t i = 0; i<PASS_LENGTH; i++)
    {
        if (password_att[i] != password[i]){
            printf("Passoword Incorrect!\n");
            printf("Digited password %s", password_att);
            *curr_pos = 0;
            return 1;
        }
    }
    printf("Correct password!\n");
    return 0;
}

void open_lock(bool *locker_opened){
    uint slice = pwm_gpio_to_slice_num(SERVO_CTL);
    pwm_set_gpio_level(SERVO_CTL, 220); //[220,1100]
    pwm_set_enabled(slice, true);
    sleep_ms(1000);
    pwm_set_enabled(slice,false);
    printf("Opening locker...\n");
    *locker_opened = true;
}
void close_lock(bool *locker_opened){
    uint slice = pwm_gpio_to_slice_num(SERVO_CTL);
    pwm_set_gpio_level(SERVO_CTL, 1100); //[220,1100]
    pwm_set_enabled(slice, true);
    sleep_ms(1000);
    pwm_set_enabled(slice,false);
    printf("Closing locker...");
    *locker_opened = false;
}

int main()
{
    uint8_t position = 0;
    uint8_t curr_position = 0;
    uint8_t password[PASS_LENGTH] = {0,0,0,0};
    bool pass_complete, correct_pass;
    bool locker_opened = false;
    bool change_pass = false;

    setup();
    sleep_ms(5000);
    printf("Locker Begin!\n");

    while (false) {
        if (locker_opened == true && change_pass == false) {
            if(gpio_get(BTN_A) == 0 ){
                change_pass = true;
                printf("set change pass\n");
            } else if (gpio_get(BTN_B) == 0 )
            {
                close_lock(&locker_opened);
            }
        } else if(gpio_get(BTN_B) == 0){
            pass_complete = get_password(&curr_position, &position);
            if (pass_complete == 0 && change_pass == true){
                change_password(password);
                printf("changing password\n");
                close_lock(&locker_opened);
                change_pass = false;
            } else if (pass_complete == 0){
                correct_pass = check_password(password, &curr_position, &locker_opened);
                if(correct_pass == 0){
                    correct_pass = 1; // 
                    open_lock(&locker_opened);
                }
            } 
        } else if(gpio_get(BTN_A) == 0){
            delete_password(&curr_position);
        }
        read_joystick_value();
        number_selector(&position);

        sleep_ms(500);
    }




}
