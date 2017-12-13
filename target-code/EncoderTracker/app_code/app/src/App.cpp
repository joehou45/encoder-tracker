//###########################################################################
// App.cpp
// Top level application class
//
// $Copyright: Copyright (C) LDO Systems
//###########################################################################
#include "App.h"

App::App()
{
	for (int i = 0; i < num_encoders; ++i)
	{
		encoderVals[i] = 0;
		oldEncoderCounts[i] = 0;
	}
	
}


///Initializes all necessary hardware
void App::Initialize()
{
	ConfigSpi();
	ConfigEncoder();
	ConfigExtInt();
}


///Configures the Timer modules to be used as encoders
void App::ConfigEncoder()
{
	const uint16_t encoder_resolution = 0xFFFF;
	Encoder::EncoderPinConfig pinConfig;

	encoderList[_X1].Initialize(Encoder::_Tim4);
	encoderList[_X2].Initialize(Encoder::_Tim1);
	encoderList[_Y].Initialize(Encoder::_Tim3);
	encoderList[_Z].Initialize(Encoder::_Tim2);

	pinConfig.chAChannel = Gpio::_ChA;
	pinConfig.chBChannel = Gpio::_ChA;
	pinConfig.chAPin = 11;
	pinConfig.chBPin = 12;
	pinConfig.chAAltNum = 10;
	pinConfig.chBAltNum = 10;
	encoderList[_X1].EncoderPinInit(pinConfig);

	pinConfig.chAChannel = Gpio::_ChA;
	pinConfig.chBChannel = Gpio::_ChA;
	pinConfig.chAPin = 8;
	pinConfig.chBPin = 9;
	pinConfig.chAAltNum = 6;
	pinConfig.chBAltNum = 6;
	encoderList[_X2].EncoderPinInit(pinConfig);

	pinConfig.chAChannel = Gpio::_ChB;
	pinConfig.chBChannel = Gpio::_ChB;
	pinConfig.chAPin = 4;
	pinConfig.chBPin = 5;
	pinConfig.chAAltNum = 2;
	pinConfig.chBAltNum = 2;
	encoderList[_Y].EncoderPinInit(pinConfig);

	pinConfig.chAChannel = Gpio::_ChA;
	pinConfig.chBChannel = Gpio::_ChA;
	pinConfig.chAPin = 0;
	pinConfig.chBPin = 1;
	pinConfig.chAAltNum = 1;
	pinConfig.chBAltNum = 1;
	encoderList[_Z].EncoderPinInit(pinConfig);

	for (int i = 0; i < num_encoders; ++i)
	{
		encoderList[i].ConfigResolution(encoder_resolution);
		encoderList[i].Enable();
		encoderList[i].Reset();
	}
}


///Configures an SPI module as slave
void App::ConfigSpi()
{
	Spi::SpiPinConfig spiPinConfig;
	spiPinConfig.sckPin = 5;
	spiPinConfig.mosiPin = 7;
	spiPinConfig.misoPin = 6;

	spiPinConfig.misoAltNum = 5;
	spiPinConfig.mosiAltNum = 5;
	spiPinConfig.sckAltNum = 5;

	spiPinConfig.misoCh = Gpio::_ChA;
	spiPinConfig.mosiCh = Gpio::_ChA;
	spiPinConfig.sckCh = Gpio::_ChA;

	spi.Initialize(Spi::SpiChannel::_Spi1);
	spi.ConfigBitMode(Spi::SpiBitMode::_TwoLine);
	spi.ConfigCrc(false);
	spi.ConfigFrame(Spi::SpiLsbFirst::_MsbFirst, Spi::SpiDataSize::_8Bit);
	spi.ConfigFifoRecThreshold(Spi::SpiRxFifoThres::_2Byte);
	spi.ConfigFrameFormat(Spi::SpiFrameFormat::_MotorolaMode);
	spi.ConfigModeAndPins(Spi::SpiMasterSel::_Slave, Spi::SpiMode::_Cpol0Cpha0, spiPinConfig);
	spi.ConfigBaudRatePrescaler(Spi::SpiBaudRate::_Fpclk8);
}


///Configures the chip select pin as an external interrupt
void App::ConfigExtInt()
{
	Gpi csPin;
	csPin.Initialize(Gpio::_ChA, 4);
	csPin.ConfigInputType(Gpio::_NoPull);

	Nvic nvic;
	nvic.Initialize(EXTI4_IRQn);
	nvic.ConfigPriorityGroupSetting(Nvic::_PriorityGroup1);
	nvic.SetPriority(0, 0);
	nvic.EnableInterrupt();

	exti.Initialize(4);
	exti.ConfigExtPin(Gpio::_ChA, 4);
	exti.ConfigMode(ExtInterrupt::_Interrupt);
	exti.ConfigTriggerEdge(ExtInterrupt::_Falling);
}


///Reads the encoder counts from the Timer modules and converts them into 32bit signed values
///This function should be called in the main loop
void App::Execute()
{
	uint16_t encCount;

	for (int i = 0; i < num_encoders; ++i)
	{
		encCount = encoderList[i].GetCounter();
		encoderVals[i] += (int16_t)(encCount - oldEncoderCounts[i]);
		oldEncoderCounts[i] = encCount;
	}
}


///Interprets and executes command from the SPI master
void App::ServeSpi()
{
	uint8_t header = 0;
	uint8_t cmd = 0;

	spi.Read(&header, 1);
	cmd = header & cmd_mask;

	if (read_cmd == cmd)
	{
		SendEncoderVals(header);
	}
	else if (reset_cmd == cmd)
	{
		ClearEncoderVals(header);
	}
}


///Transmits selected encoder values to the SPI master
void App::SendEncoderVals(uint8_t header)
{
	uint8_t encSelector = header >> enc_offset;
	uint16_t numTxEncoders = 0;
	int32_t txBuffer[num_encoders];

	for (int i = 0; i < num_encoders; ++i)
	{
		if (encSelector & 0x01)
		{
			txBuffer[numTxEncoders++] = encoderVals[i];
		}
		encSelector >> 1;
	}

	spi.Write( (uint8_t*)txBuffer, numTxEncoders * sizeof(int32_t) );
}


///Resets selected encoder values to 0
void App::ClearEncoderVals(uint8_t header)
{
	uint8_t encSelector = header >> enc_offset;

	for (int i = 0; i < num_encoders; ++i)
	{
		if (encSelector & 0x01) encoderVals[i] = 0;
		encSelector >> 1;
	}
}

