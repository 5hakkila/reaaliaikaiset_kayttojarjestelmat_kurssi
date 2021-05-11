#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include "dht22data.h"

//Random-arvojen generointi
bool read_data(dht22Data *data);
//Signaalin käsittely
void read_shared_mem();

int main(int argc, char** argv) {

    //Dht22Data-Osoitin jaettuun muistiin
    dht22Data *shared_mem = (dht22Data*) mmap(NULL,
                            sizeof(dht22Data),
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS,
                            -1,
                            0);

    //Luodaan uusi prosessi
    pid_t pid = fork();

    //Anturidatan lukeminen reaaliaikaisesti
    if(pid == 0){

        //Prosessin priorisointi, 
        const struct sched_param priority = {1};
        sched_setscheduler(0, SCHED_FIFO, &priority);

        //Lukitaan prosessin virtuaalinen muistiavaruus
        mlockall(MCL_CURRENT | MCL_FUTURE);

        //DHT22-data(Lämpötila, kosteus)
        dht22Data dht22_data;

        //Luetaan anturidataa sekunnin välein
        while(true){

            //1 sekunti sleep
            sleep(2);

            //Luetaan uudet arvot
            if(read_data(&dht22_data)){

                //Kopioidaan uudet arvot jaettuun muistiin
                memcpy(shared_mem, &dht22_data, sizeof(dht22_data));

                //printf("p1: temperature: %f \n", dht22_data.temperature);
                //printf("p1: humidity: %f\n", dht22_data.humidity);
            }
        }   
    }

    //Ei-reaaliaikainen prosessi, datan lähettäminen pilveen
    else if(pid > 0){

        float prev_temperature_val = 0.f;
        float prev_humidity_val    = 0.f;

        //Pyöritään tässä silmukassa
        while(true){

            sleep(1);

            //Uusi lämpötilarvo jaetussa muistissa
            if(prev_temperature_val != shared_mem->temperature){

                printf("p2 temp: %f\n", shared_mem->temperature);
                prev_temperature_val = shared_mem->temperature;
            }
        }
    }

    return 0;
}

//Mittausdatan luominen randomisti
bool read_data(dht22Data *data){

    //Random seed
    time_t t;
    srand(time(&t));

    //Järkevät random-arvot
    float currentTemperature = (2100 + rand() % 150) / 100.0;
    float currentHumidity    = (4500 + rand() % 250) / 100.0;

    //Uudet arvot structin jäseniin
    data->humidity    = currentHumidity;
    data->temperature = currentTemperature;

    return true;
}