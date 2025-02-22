#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "ctype.h"
#include "string.h"

#define BTN_A 5
#define BTN_B 6
#define I2C_SDA 14
#define I2C_SCL 15
#define VRX 27
#define VRY 26
#define ADC_CHANNEL_0 0
#define ADC_CHANNEL_1 1
#define SW 22
#define SERVO_CTL 28
#define PWM_DIVIDER 250.0f
#define PERIOD 10000
#define PASS_LENGTH 4

uint8_t password_att[PASS_LENGTH];
uint16_t vry_num=0;
char curr_password_text[17];
char* message;


void write_message(char** text, size_t x, size_t y, struct render_area* frame_area, uint8_t* ssd, size_t size)
{
    for (uint i = 0; i < size; i++)
    {
        // printf(text[i]);
        ssd1306_draw_string(ssd, x, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, frame_area);
}

void selected_number(uint num)
{
    uint8_t num_before;
    uint8_t num_after; 

    if(num == 0)
        num_before = 9;
    else 
        num_before = num - 1;
    if(num==9)
        num_after = 0;
    else
        num_after = num + 1;
    
    sprintf(message, "   %d [%d] %d   ", num_before, num, num_after);
}


void setup_i2c()
{
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();
}

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
    // setup_i2c();
}

void read_joystick_value()
{
    adc_select_input(ADC_CHANNEL_1);
    vry_num = adc_read();
}

void number_selector(uint8_t *position, char** text, bool* change, struct render_area *frame_area, uint8_t* ssd)
{
    // sleep_ms(100);
    if(vry_num < ((4096/2)-200)) //verify if goes left
    {
        *change = true;
        if(*position == 0){
            *position = 9;
        } else {
            (*position)--;
        }
    } 
    else if(vry_num > ((4096/2)+200)) //verify if goes right
    {
        *change = true;
        if(*position == 9){
            *position = 0;
        } else {
            (*position)++;
        }
    }
    if(*change){
        printf("New position: %d\n", *position);
        selected_number(*position);
        memset(ssd, 0, ssd1306_buffer_length);
        write_message(text, 5, 0, frame_area, ssd, count_of(text));
        char** ptr = &message;
        char* text_under[] = {text[2], text[3]};
        write_message(ptr, 5, 8, frame_area, ssd, count_of(ptr));
        write_message(text_under, 5, 16, frame_area, ssd, count_of(text_under));
        *change = false;
    }
}

bool get_password(uint8_t* curr_pos, uint8_t* position, struct render_area *frame_area, uint8_t* ssd)
{
    password_att[*curr_pos] = *position;
    if (*curr_pos == PASS_LENGTH - 1){
        *curr_pos = 0; // reset to accept new attempt
        return 0;
    }
    (*curr_pos)++;
    printf("Get new char!\n");
    char* msg[] = {"       ok       "};
    write_message(msg, 5, 32, frame_area, ssd, count_of(msg));
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
            printf("Password Incorrect!\n");
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
    sleep_ms(200);
}
void close_lock(bool *locker_opened){
    uint slice = pwm_gpio_to_slice_num(SERVO_CTL);
    pwm_set_gpio_level(SERVO_CTL, 1100); //[220,1100]
    pwm_set_enabled(slice, true);
    sleep_ms(1000);
    pwm_set_enabled(slice,false);
    printf("Closing locker...");
    *locker_opened = false;
    sleep_ms(200);
}

int main()
{
    uint8_t position = 0;
    uint8_t curr_position = 0;
    uint8_t password[PASS_LENGTH] = {0,0,0,0};
    bool pass_complete, correct_pass;
    bool locker_opened = false;
    bool change_pass = false;
    message = (char*)malloc(sizeof(char)*18);

    setup_i2c();

    
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };
    
    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    
    
    char* get_pass_text[] = {
        " Digite a senha:",
        " ", //change on selection change
        " A  corrigir",
        " B  confirmar"
    };
    char* change_pass_text[] = {
        "Alterar a senha?",
        "   A  Sim    ",
        "   B  Nao    "
    };
    char* correct_pass_text[] = {
        "Senha correta",
        "Abrindo cofre..."
    };
    char* incorrect_pass_text[] = {
        "Senha incorreta",
        "tente novamente"
    };
    char* close_lock_text[] = {
        "Fechando..."
    };
    char* pass_changed_text[] = {
        "Senha alterada"
    };
    
    
    setup();
    sleep_ms(5000);
    printf("Locker Begin!\n");
    bool change = true;
    while (true) {
        if (locker_opened == true && change_pass == false) {
            write_message(change_pass_text, 5, 0, &frame_area, ssd, count_of(change_pass_text));
            sleep_ms(300);
            if(gpio_get(BTN_A) == 0 ){
                change_pass = true;
                change = true;

                printf("set change pass\n");
            } else if (gpio_get(BTN_B) == 0 )
            {
                memset(ssd,0,ssd1306_buffer_length);
                write_message(close_lock_text, 5, 0, &frame_area, ssd, count_of(close_lock_text));
                close_lock(&locker_opened);
                change = true;
            }
        } else if(gpio_get(BTN_B) == 0){
            pass_complete = get_password(&curr_position, &position, &frame_area, ssd);
            if (pass_complete == 0 && change_pass == true){
                change_password(password);
                printf("changing password\n");
                write_message(change_pass_text, 5, 0, &frame_area, ssd, count_of(change_pass_text));
                memset(ssd,0,ssd1306_buffer_length);
                write_message(close_lock_text, 5, 8, &frame_area, ssd, count_of(close_lock_text));
                close_lock(&locker_opened);
                change_pass = false;
            } else if (pass_complete == 0){
                correct_pass = check_password(password, &curr_position, &locker_opened);
                if(correct_pass == 0){
                    correct_pass = 1; // 
                    memset(ssd, 0, ssd1306_buffer_length);
                    write_message(correct_pass_text, 5, 0, &frame_area, ssd, count_of(correct_pass_text));
                    open_lock(&locker_opened);
                } else {
                    memset(ssd, 0, ssd1306_buffer_length);
                    write_message(incorrect_pass_text, 5, 0, &frame_area, ssd, count_of(incorrect_pass_text));
                }
            } 
        } else if(gpio_get(BTN_A) == 0){
            delete_password(&curr_position);
        }
        read_joystick_value();
        number_selector(&position, get_pass_text, &change, &frame_area, ssd);

        sleep_ms(500);
    }


    number_selector(&position, get_pass_text, &change, &frame_area, ssd);
    write_message(get_pass_text,5,0,&frame_area, ssd, count_of(get_pass_text));
    selected_number(1);
    char** ptr = &message;
    write_message(ptr, 5, 8, &frame_area, ssd, count_of(ptr));


}
