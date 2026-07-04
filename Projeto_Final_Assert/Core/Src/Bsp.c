/******************************************************************************/
/**
 * @file Bsp.c
 * @addtogroup Bsp
 * @author Luis Eduardo
 * @details
 * @version 1
 * @} DOXYGEN GROUP TAG END OF FILE
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Bsp.h"
#include "main.h"
#include <string.h>
#include <stdbool.h> /* Adicionado para suporte ao tipo bool */
#include <stdint.h>  /* Adicionado para os tipos padroes do C */

/*******************************************************************************
 * DEFINES LOCAIS
 ******************************************************************************/
/// Timeout para leitura do ADC em milissegundos
#define dADC_TIMEOUT_MS  10
/// Timeout para envio da UART em milissegundos
#define dUART_TIMEOUT_MS 100
/// Porcentagem maxima
#define dPERCENTAGE_MAX 100
/// Quantidade de bytes para UART
#define dUART_RX_BYTES_QTY 1

/*******************************************************************************
 * CONSTANTES
 ******************************************************************************/

/*******************************************************************************
 * ESTRUTURAS DE DADOS LOCAIS
 ******************************************************************************/
/// Variaveis internas de controle da Bsp
static struct
{
    /// Flag para amostragem do ADC (5ms)
    bool isSamplingReady;

    /// Flag para detecao final do botao apos debounce
    bool isButtonPressed;

    /// Flag de recepcao da UART
    bool isUartDataReady;

    /// Buffer de recepcao de 1 byte da USART3
    u8 uartRxBuffer;
} bsp;

/* Handles gerados pelo STM32CubeMX para a F767ZI */
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim6;   // Timer Amostragem (5 ms)
extern TIM_HandleTypeDef htim7;   // Timer Debounce
extern TIM_HandleTypeDef htim3;   // Timer PWM dos LEDs
extern UART_HandleTypeDef huart3; // USART3 (ST-Link VCP)

/*******************************************************************************
 * PROTOTIPOS LOCAIS
 ******************************************************************************/

/*******************************************************************************
 * FUNCOES PUBLICAS
 ******************************************************************************/

/******************************************************************************/
/** @brief Inicializa perifericos. UART e PWM por Polling. Apenas TIM6 usa IT.
 * @retval Nenhum.
 ******************************************************************************/
void Bsp_Init(void)
{
	bsp.isSamplingReady = false;
	bsp.isButtonPressed = false;
	bsp.isUartDataReady = false;
	bsp.uartRxBuffer = 0;

	/* Inicia o Timer de Amostragem (ÚNICA INTERRUPÇÃO PERMITIDA) */
	HAL_TIM_Base_Start_IT(&htim6);

	/* Inicia os canais de PWM sem interrupção */
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

	/* === ADICIONE ESTA LINHA AQUI PARA LIGAR A ESCUTA DA UART === */
	HAL_UART_Receive_IT(&huart3, &bsp.uartRxBuffer, dUART_RX_BYTES_QTY);
}

/******************************************************************************/
/** @brief Aplica o valor de duty cycle ao LED.
 * PWM manual via registro CCR (Polling).
 ******************************************************************************/
void Bsp_SetLedPwm(bspLed_t ledChannel, u8 dutyPercent)
{
    // Obtem o valor do ARR para calcular a proporção correta
    u32 maxArr = __HAL_TIM_GET_AUTORELOAD(&htim3);

    // Calcula o valor do registrador de comparação (CCR) manualmente
    // dutyPercent (0-100) * maxArr / 100
    u32 ccrValue = (maxArr * dutyPercent) / dPERCENTAGE_MAX;

    switch(ledChannel)
    {
        case eBSP_LED_1:
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccrValue);
            break;
        case eBSP_LED_2:
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, ccrValue);
            break;
        case eBSP_LED_3:
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, ccrValue);
            break;
        default:
            break;
    }
}

/******************************************************************************/
/** @brief Retorna o estado atual da flag de amostragem.
 * @retval true se a amostragem estiver pronta, false caso contrário.
 ******************************************************************************/
bool Bsp_GetSamplingFlag(void)
{
    return bsp.isSamplingReady;
}

/******************************************************************************/
/** @brief Limpa a flag de amostragem.
 * @retval Nenhum.
 ******************************************************************************/
void Bsp_ClearSamplingFlag(void)
{
    bsp.isSamplingReady = false;
}

/******************************************************************************/
/** @brief Realiza a leitura do canal ADC via polling.
 * @retval Valor digital lido do ADC (12 bits).
 ******************************************************************************/
u16 Bsp_ReadAdc(void)
{
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, dADC_TIMEOUT_MS) == HAL_OK)
    {
        return (u16)HAL_ADC_GetValue(&hadc1);
    }
    return 0;
}

/******************************************************************************/
/** @brief Transmite uma string via UART usando polling.
 * @param str: Ponteiro para a string a ser transmitida.
 * @retval Nenhum.
 ******************************************************************************/
void Bsp_TransmitUartString(const char *str)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)str, strlen(str), dUART_TIMEOUT_MS);
}

/******************************************************************************/
/** @brief Retorna o estado da flag do botão.
 * @retval true se o botão foi pressionado, false caso contrário.
 ******************************************************************************/
bool Bsp_GetButtonFlag(void)
{
    return bsp.isButtonPressed;
}

/******************************************************************************/
/** @brief Limpa a flag de detecção do botão.
 * @retval Nenhum.
 ******************************************************************************/
void Bsp_ClearButtonFlag(void)
{
    bsp.isButtonPressed = false;
}

/******************************************************************************/
/** @brief Verifica se há dados novos na UART.
 * @retval true se houver dados, false caso contrário.
 ******************************************************************************/
bool Bsp_GetUartRxFlag(void)
{
    return bsp.isUartDataReady;
}

/******************************************************************************/
/** @brief Limpa a flag de recepção da UART.
 * @retval Nenhum.
 ******************************************************************************/
void Bsp_ClearUartRxFlag(void)
{
    bsp.isUartDataReady = false;
}

/******************************************************************************/
/** @brief Retorna o dado recebido pela UART.
 * @retval Byte recebido.
 ******************************************************************************/
u8 Bsp_GetUartRxData(void)
{
    return bsp.uartRxBuffer;
}
/*******************************************************************************
 * FUNCOES LOCAIS (CALLBACKS DA HAL)
 ******************************************************************************/

/******************************************************************************/
/** @brief Callback de interrupcao de pinos externos (EXTI).
 * @param GPIO_Pin: Pino que gerou a interrupcao.
 * @retval Nenhum.
 ******************************************************************************/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == USER_Btn_Pin)
    {
        HAL_TIM_Base_Start_IT(&htim7);
    }
}

/******************************************************************************/
/** @brief Callback padrao da HAL para estouro de Timers.
 * Apenas TIM6 (Amostragem) e TIM7 (Debounce) devem disparar aqui.
 ******************************************************************************/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // Apenas o TIM6 sinaliza a amostragem
    if(htim->Instance == TIM6)
    {
        bsp.isSamplingReady = true;
    }
    // O TIM7 é usado apenas para o tempo de espera do debounce
    else if(htim->Instance == TIM7)
    {
        HAL_TIM_Base_Stop_IT(&htim7); // Para o contador após o tempo de debounce

        if(HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET)
        {
            bsp.isButtonPressed = true;
        }
    }
}

/******************************************************************************/
/** @brief Callback da HAL para recepção completa de dados na UART.
 ******************************************************************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)
    {
        bsp.isUartDataReady = true;
        // Reinicia a escuta para o próximo byte
        HAL_UART_Receive_IT(&huart3, &bsp.uartRxBuffer, dUART_RX_BYTES_QTY);
    }
}

/** @} */
