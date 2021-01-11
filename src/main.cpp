/*
 * Copyright 2020, Energy Informatics Group, LUMS University. All rights reserved.
 * Developed by Abdur Rahman(rao.arrn@gmail.com), Wajahat Ali(s_ali@lums.edu.pk).
 * 
 * Main file for Battery Swapping Station code
 * 
 * This file is the property of Energy Informaics Group, Lums University. Its sole private
 * ownership remains with LUMS and it should not be shared with anyone without the consent 
 * of respective authority.
 */

//TODO: handle the problem when semaphore is not available within the defined time. and also define the blocking time
//TODO: OPTIMIZATION: convert String to c string.
#define DATA_ACQUISITION_TIME 1000      //perform action every 1000ms
#define DATA_MAX_LEN 1200   //bytes

#include <Arduino.h>
#include <FreeRTOS.h>
#include <rtc.h>
#include <Storage.h>
#include <can.h>
#include <bluetooth.h>
#include "esp32-mqtt.h"

String towrite, toread;
TaskHandle_t dataTask, blTask, storageTask, wifiTask;
void vAcquireData( void *pvParameters );
void vBlTransfer( void *pvParameters );
void vStorage( void *pvParameters );
void vWifiTransfer( void *pvParameters );

SemaphoreHandle_t semaAqData1, seamAqData2, semaBlTx1, semaStorage1, seamStorage2, semaWifi1;
void addSlotsData(String B_Slot,String B_ID,String B_Auth, String B_Age,String B_Type ,String B_M_Cycles ,String B_U_Cycles , 
                    String B_Temp, String B_SoC, String B_SoH, String B_RoC,String B_Vol ,String B_Curr){
    towrite += B_Slot + "," + B_ID + "," + B_Auth + "," + B_Age + "," + B_Type + "," + B_M_Cycles + "," + 
            B_U_Cycles + "," + B_Temp + "," + B_SoC + "," + B_SoH + "," + B_RoC + "," + B_Vol + "," + B_Curr;
    return;
}



void setup() {
    Serial.begin(115200); //Start Serial monitor
    pinMode(LED_BUILTIN, OUTPUT);
    //setupCloudIoT();
    bt.init();
    initRTC();
    if(storage.init_storage()){
        Serial.println("main() -> main.cpp -> storage initialization success!");
    }
    else{   //TODO: handle when storage connection fails
        while(1){
            Serial.println("main() -> main.cpp -> storage initialization failed!");
            delay(1000);
        }
    }
    semaAqData1 = xSemaphoreCreateBinary();
    seamAqData2 = xSemaphoreCreateBinary();
    semaBlTx1 = xSemaphoreCreateBinary();
    semaStorage1 = xSemaphoreCreateBinary();
    seamStorage2 = xSemaphoreCreateBinary();
    semaWifi1 = xSemaphoreCreateBinary();
    xSemaphoreGive(semaAqData1);
    xSemaphoreGive(seamAqData2);
    xSemaphoreGive(semaWifi1);
    
    xTaskCreatePinnedToCore(vAcquireData, "Data Acquisition", 10000, NULL, 2, &dataTask, 0);
    xTaskCreatePinnedToCore(vBlTransfer, "Bluetooth Transfer", 100000, NULL, 1, &blTask, 0);
    xTaskCreatePinnedToCore(vStorage, "Storage Handler", 10000, NULL, 2, &storageTask, 1);
    xTaskCreatePinnedToCore(vWifiTransfer, "Transfer data on Wifi", 100000, NULL, 1, &wifiTask, 1);
    Serial.println("created all tasks");
}
unsigned long lastMillis = 0;
String CloudData = "";
void loop() {
    
    /*if(WiFi.status() == WL_CONNECTED){digitalWrite(2, 1);}
    
    
    
    
    mqtt->loop();
    delay(10);  // <- fixes some issues with WiFi stability
    if (!mqttClient->connected()) {
        connect();
    }
    if (mqttClient->connected()) {
        Serial.println("*****");
        Serial.println(publishTelemetry(towrite));
        Serial.println(ESP.getFreeHeap());
        //Serial.println(publishTelemetry("hi"));
        Serial.println("*****");
    }
    storage.write_data(getTime2(), towrite);
    towrite= "";
    towrite = storage.read_data();
    if(towrite != ""){
        storage.write_data("copy",towrite);
        storage.mark_data(getTime2());
    }
    Serial.println(towrite);
    static long counter = 0;
    counter++;
    Serial.println(counter);
    Serial.println();
    delay(1000);*/
}
void vAcquireData( void *pvParameters ){
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = DATA_ACQUISITION_TIME;
    xLastWakeTime = xTaskGetTickCount();

    for(;;){    //infinite loop
        xSemaphoreTake(semaAqData1, portMAX_DELAY); //semaphore to check if sending of data over bluetooth and storage has returned
        xSemaphoreTake(seamAqData2, portMAX_DELAY);
        if(WiFi.status() == WL_CONNECTED)   digitalWrite(2, !digitalRead(2));   //LED will blink when there is connection
        {
            //Dummy acquisition of data
            //we need to place a valid CSV string in towrite string
            towrite = "";
            towrite += getTime() + ",";                 //time
            towrite += String("BSS1715001") + ",";      //BSSID
            towrite += String("16") + ",";              //total slots
            towrite += String("20.273") + ",";              //BSS voltage
            towrite += String("55.781") + ",";              //BSS CURRENT
            towrite += String("7400") + ",";                  //BSS POWER
            towrite += String("0.8") + ",";                 //BSS power factor
            //addSlotsData(String B_Slot,String B_ID,String B_Auth, String B_Age,String B_Type ,String B_M_Cycles ,
            //             String B_U_Cycles , String B_Temp, String B_SoC, String B_SoH, String B_RoD,String B_Vol ,String B_Curr)
            addSlotsData("01", "1718953129", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("02", "1718953130", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("03", "1718953131", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("04", "1718953128", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "26.561");towrite += ",";
            addSlotsData("05", "1718953127", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("06", "1718953126", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("07", "1718953125", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("08", "1718953124", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "26.561");towrite += ",";
            addSlotsData("09", "1718953123", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("10", "1718953122", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("11", "1718953121", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("12", "1718953120", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "26.561");towrite += ",";
            addSlotsData("13", "1718953119", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("14", "2718953129", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("15", "1518953129", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "20.561");towrite += ",";
            addSlotsData("16", "1718253129", "BSS22", "22", "2211", "500", "200", "30", "80", "50", "22", "12.371", "26.561");
            //Now towrite string contains one valid string of CSV data chunk
        }
        xSemaphoreGive(semaBlTx1);      //signal to call bluetooth transfer function once
        xSemaphoreGive(semaStorage1);   //signal to call storage save data function once
        vTaskDelayUntil(&xLastWakeTime, xFrequency);    //defines the data acquisition rate
    }   //end for
}   //end vAcquireData

void vBlTransfer( void *pvParameters ){ //synced by the acquire data function
    for(;;){    //infinite loop
        xSemaphoreTake(semaBlTx1,portMAX_DELAY);
        {
            bt.send(towrite);
        }
        xSemaphoreGive(semaAqData1);
    }   //end for
}   //end vBlTransfer task

void vStorage( void *pvParameters ){
    for(;;){    //infinite loop
        xSemaphoreTake(semaStorage1,portMAX_DELAY);
        xSemaphoreTake(semaWifi1,portMAX_DELAY);
        {
            storage.write_data(getTime2(), towrite);
        }
        xSemaphoreGive(semaWifi1);  //resume the wifi transfer task
        xSemaphoreGive(seamAqData2);
    }   //end for
}   //end vStorage task

void vWifiTransfer( void *pvParameters ){
    for(;;){    //infinite loop
        //check unsent data and send data over wifi
        //also take semaWifi1 when starting to send one chunk of data and give semaWifi1 when sending of one chunk of data is complete
        xSemaphoreTake(semaWifi1,portMAX_DELAY);
        /*{
            mqtt->loop();
            delay(10);  // <- fixes some issues with WiFi stability
            if (!mqttClient->connected()) {
                connect();
            }
            if (mqttClient->connected()) {
                Serial.println("*****");
                Serial.println(publishTelemetry(towrite));
                Serial.println(ESP.getFreeHeap());
                //Serial.println(publishTelemetry("hi"));
                Serial.println("*****");
            }
        }*/
        xSemaphoreGive(semaWifi1);
    }   //end for
}   //end vWifiTransfer task