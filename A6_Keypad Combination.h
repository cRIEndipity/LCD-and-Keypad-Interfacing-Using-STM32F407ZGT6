#include "main.h"
#include "lcd_i2c.h"
#include "keypad.h"

I2C_HandleTypeDef hi2c1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();

    // Initialize LCD and Keypad
    LCD_Init(&hi2c1);
    Keypad_Init();

    // Display initial message
    LCD_Clear(&hi2c1);
    LCD_SetCursor(&hi2c1, 0, 0);
    LCD_SendString(&hi2c1, "Press any key: ");

    char current_key = '\0';
    char last_key = '\0';
    char sequence[17] = ""; // Buffer to hold the sequence (max 16 chars + null terminator)
    uint8_t seq_length = 0;
    uint8_t combination_mode = 0; // 0=normal, 1=waiting for second digit after '1'

    while (1)
    {
        current_key = Keypad_GetKey();

        if (current_key != '\0' && current_key != last_key)
        {
            // Handle combination mode first
            if (combination_mode == 1)
            {
                if (current_key == '4')
                {
                    // 1 then 4 → E
                    if (seq_length < 16) {
                        sequence[seq_length++] = 'E';
                    }
                }
                else if (current_key == '5')
                {
                    // 1 then 5 → F
                    if (seq_length < 16) {
                        sequence[seq_length++] = 'F';
                    }
                }
                else
                {
                    // If not 4 or 5, add both '1' and the current key normally
                    if (seq_length < 16) {
                        sequence[seq_length++] = '1';
                    }
                    if (current_key == '1') {
                        combination_mode = 1;
                    }
                    else if (seq_length < 16) {
                        sequence[seq_length++] = current_key;
                    }
                }
                if (current_key != '1') {
                    combination_mode = 0; // Reset combination mode
                }
            }
            else
            {
                // Check if this key starts a combination
                if (current_key == '1')
                {
                    combination_mode = 1; // Enter combination mode, don't add '1' yet
                }
                else
                {
                    // Normal key press - add to sequence
                    if (seq_length < 16) {
                        sequence[seq_length++] = current_key;
                    }
                }
            }

            // Update display
            LCD_SetCursor(&hi2c1, 1, 0);
            LCD_SendString(&hi2c1, "                "); // Clear entire line
            LCD_SetCursor(&hi2c1, 1, 0);

            // Display the entire sequence
            sequence[seq_length] = '\0'; // Ensure null termination
            LCD_SendString(&hi2c1, sequence);

            // Update last_key for debouncing
            last_key = current_key;
        }
        else if (current_key == '\0')
        {
            last_key = '\0';
        }

        // Small delay to prevent flooding
        HAL_Delay(100);
    }
}
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_I2C1_Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}