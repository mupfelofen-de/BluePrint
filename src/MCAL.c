// SPDX-License-Identifier: Beerware
/**
 * @file      MCAL.c
 * @brief     Microcontroller Abstraction Layer
 * @details   MCAL for STM32F1xx microcontroller
 * @author    Michael Fitzmayer
 * @copyright "THE BEER-WARE LICENCE" (Revision 42)
 */

#include <stdbool.h>
#include <stdint.h>
#include "MCAL.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include "stm32f1xx_hal_spi.h"
#include "stm32f1xx_hal_tim.h"

extern I2C_HandleTypeDef hi2c2;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim1;

static GPIO_TypeDef* _MCAL_ConvertGPIOPort(GPIOPort ePort);

/**
 * @brief  Read current input pin state
 * @param  ePort
 *         GPIO port
 * @param  u16PinMask
 *         Pin mask
 * @return Boolean state
 * @retval true: Pin is high
 * @retval false: Pin is low
 */
bool GPIO_IsSet(GPIOPort ePort, uint16_t u16PinMask)
{
    GPIO_TypeDef* phPort = _MCAL_ConvertGPIOPort(ePort);

    if (HAL_GPIO_ReadPin(phPort, u16PinMask))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Pull output pin(s) low
 * @param ePort
 *        GPIO port
 * @param u16PinMask
 *        Pin mask
 */
void GPIO_PullDown(GPIOPort ePort, uint16_t u16PinMask)
{
    GPIO_TypeDef* phPort = _MCAL_ConvertGPIOPort(ePort);
    HAL_GPIO_WritePin(phPort, u16PinMask, GPIO_PIN_RESET);
}

/**
 * @brief Raise output pin(s) high
 * @param ePort
 *        GPIO port
 * @param u16PinMask
 *        Pin mask
 */
void GPIO_RaiseHigh(GPIOPort ePort, uint16_t u16PinMask)
{
    GPIO_TypeDef* phPort = _MCAL_ConvertGPIOPort(ePort);
    HAL_GPIO_WritePin(phPort, u16PinMask, GPIO_PIN_SET);
}

/**
 * @brief Toggle output pin(s) between high and low
 * @param ePort
 *        GPIO port
 * @param u16PinMask
 *        Pin mask
 */
void GPIO_Toggle(GPIOPort ePort, uint16_t u16PinMask)
{
    GPIO_TypeDef* phPort = _MCAL_ConvertGPIOPort(ePort);
    HAL_GPIO_TogglePin(phPort, u16PinMask);
}

/**
 * @brief  Receive an amount via I²C
 * @param  u16DevAddress
 *         Target device address
 * @param  u16MemAddress
 *         Internal memory address
 * @param  eMemAddSize
 *         Memory address size
 * @param  pu8RxBuffer
 *         Pointer to data buffer
 * @param  u16Size
 *         Amount of data to be sent
 * @return Error code
 * @retval  0: OK
 * @retval -1: Error
 */
int I2C_Receive(uint16_t u16DevAddress, uint16_t u16MemAddress, I2CMemAddSize eMemAddSize, uint8_t* pu8RxBuffer, uint16_t u16Size)
{
    uint16_t u16MemAddSize = 0;

    switch (eMemAddSize)
    {
        case I2C_MEMSIZE_8BIT:
            u16MemAddSize = I2C_MEMADD_SIZE_8BIT;
            break;
        case I2C_MEMSIZE_16BIT:
            u16MemAddSize = I2C_MEMADD_SIZE_16BIT;
            break;
    }

    if (HAL_OK != HAL_I2C_Mem_Read_IT(
            &hi2c2,
            u16DevAddress,
            u16MemAddress,
            u16MemAddSize,
            pu8RxBuffer,
            u16Size))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  Transmit an amount via I²C
 * @param  u16DevAddress
 *         Target device address
 * @param  u16MemAddress
 *         Internal memory address
 * @param  eMemAddSize
 *         Memory address size
 * @param  pu8TxBuffer
 *         Pointer to data buffer
 * @param  u16Size
 *         Amount of data to be sent
 * @return Error code
 * @retval  0: OK
 * @retval -1: Error
 */
int I2C_Transmit(uint16_t u16DevAddress, uint16_t u16MemAddress, I2CMemAddSize eMemAddSize, uint8_t* pu8TxBuffer, uint16_t u16Size)
{
    uint16_t u16MemAddSize = 0;

    switch (eMemAddSize)
    {
        case I2C_MEMSIZE_8BIT:
            u16MemAddSize = I2C_MEMADD_SIZE_8BIT;
            break;
        case I2C_MEMSIZE_16BIT:
            u16MemAddSize = I2C_MEMADD_SIZE_16BIT;
            break;
    }

    if (HAL_OK != HAL_I2C_Mem_Write_IT(
            &hi2c2,
            u16DevAddress,
            u16MemAddress,
            u16MemAddSize,
            pu8TxBuffer,
            u16Size))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief   Wait for the end of the transfer
 * @details Before starting a new communication transfer, the CPU need
 *          to check the current state of the peripheral; if it's busy
 *          the CPU need to wait for the end of current transfer before
 *          starting a new one.
 * @param   u16DevAddress
 *          Target device address
 */
void I2C_WaitUntilReady(uint16_t u16DevAddress)
{
    while (HAL_I2C_STATE_READY != HAL_I2C_GetState(&hi2c2));

    while (HAL_TIMEOUT == HAL_I2C_IsDeviceReady(
               &hi2c2,
               u16DevAddress,
               100,
               100));

    while (HAL_I2C_STATE_READY != HAL_I2C_GetState(&hi2c2))
    {
        MCAL_Sleep(1);
    }
}

/**
 * @brief Microsecond delay (blocking)
 * @param u16DelayInUs
 *        Delay in microseconds
 */
void MCAL_Sleep(uint16_t u16DelayInUs)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (u16DelayInUs > __HAL_TIM_GET_COUNTER(&htim1));
}

/**
 * @brief  Transmit an amount via SPI
 * @param  pu8TxData
 *         Pointer to data buffer
 * @param  u16Size
 *         Amount of data to be sent
 * @return Error code
 * @retval  0: OK
 * @retval -1: Error
 */
int SPI_Transmit(uint8_t* pu8TxData, uint16_t u16Size)
{
    if (HAL_OK != HAL_SPI_Transmit_IT(&hspi1, pu8TxData, u16Size))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  Receive an amount of data via SPI
 * @param  pu8RxData
 *         Pointer to data buffer
 * @param  u16Size
 *         Amount of data to be sent
 * @return Error code
 * @retval  0: OK
 * @retval -1: Error
 */
int SPI_Receive(uint8_t* pu8RxData, uint16_t u16Size)
{
    if (HAL_OK != HAL_SPI_Receive_IT(&hspi1, pu8RxData, u16Size))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  Transmit and Receive an amount of data via SPI
 * @param  pu8TxData
 *         Pointer to transmission data buffer
 * @param  pu8RxData
 *         Pointer to reception data buffer
 * @param  u16Size
 *         Amount of data to be sent and received
 * @return Error code
 * @retval  0: OK
 * @retval -1: Error
 */
int SPI_TransmitReceive(uint8_t* pu8TxData, uint8_t* pu8RxData, uint16_t u16Size)
{
    if (HAL_OK != HAL_SPI_TransmitReceive_IT(&hspi1, pu8TxData, pu8RxData, u16Size))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  Convert GPIOPort to STM32_HAL's GPIO_TypeDef
 * @param  ePort
 *         GPIO port
 * @return Pointer to respective GPIO_TypeDef
 */
static GPIO_TypeDef* _MCAL_ConvertGPIOPort(GPIOPort ePort)
{
    GPIO_TypeDef* phPort = GPIOA;

    switch (ePort)
    {
        case GPIO_PORT_A:
            phPort = GPIOA;
            break;
        case GPIO_PORT_B:
            phPort = GPIOB;
            break;
        case GPIO_PORT_C:
            phPort = GPIOC;
            break;
        case GPIO_PORT_D:
            phPort = GPIOD;
            break;
    }

    return phPort;
}
