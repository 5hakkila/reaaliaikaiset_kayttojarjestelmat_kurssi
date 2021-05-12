#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include "dht22data.h"

bool new_value = false;

//Random-arvojen generointi
bool read_data(dht22Data *data);

int main(int argc, char** argv) {

    //Luodaan fifo, tarkistetaan virhekoodit
    if(mkfifo("sensoridata", 0777) == -1){

        //Ei voida luoda fifoa
        if(errno != EEXIST){
            printf("Fifon luominen epäonnistui\n");
            exit(1);
        }

        else{
            //Fifo on jo luotu, jatketaan.
        }
    }
    //Dht22Data-Osoitin jaettuun muistiin
    /*dht22Data *shared_mem = (dht22Data*) mmap(NULL,
                            sizeof(dht22Data),
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS,
                            -1,
                            0); */

    //Luodaan uusi prosessi
    pid_t pid = fork();

    //Tarkistetaan prosessin luominen
    if(pid < 0){
        printf("Ei voitu luoda uutta prosessia\n");
        exit(1);
    }

    //Prosessi anturidatan lukemiseen reaaliaikaisesti
    if(pid == 0){

        //Prosessin priorisointi
        const struct sched_param priority = {1};
        sched_setscheduler(0, SCHED_FIFO, &priority);

        //Lukitaan prosessin virtuaalinen muistiavaruus
        mlockall(MCL_CURRENT | MCL_FUTURE);

        //DHT22-data(Lämpötila, kosteus)
        dht22Data dht22_data;

        //Avataan fifo kirjoittamista varten
        int fd = open("sensoridata", O_WRONLY);

        //Luetaan anturidataa sekunnin välein
        while(true){

            //1 sekunti sleep
            sleep(1);

            char tmpTemperature[30] = "";
            //Luetaan uudet arvot
            if(read_data(&dht22_data)){

                //char tmpTemperature[25], tmpHumidity[12];

                sprintf(tmpTemperature, "%f ", dht22_data.temperature);
                //fprintf(tmpHumidity, "%f", dht22_data.humidity);
                //ftoa(dht22_data.temperature, tmpTemperature, 2);

                //strcat(tmpTemperature, tmpHumidity);

                write(fd, tmpTemperature, sizeof(tmpTemperature));
                //Kopioidaan uudet arvot jaettuun muistiin
                //memcpy(shared_mem, &dht22_data, sizeof(dht22_data));

                //printf("p1: temperature: %f \n", dht22_data.temperature);
                //printf("p1: humidity: %f\n", dht22_data.humidity);
            }
        }

        close(fd);
    }

    //Ei-reaaliaikainen prosessi, datan lähettäminen pilveen
    else if(pid > 0){

        float prev_temperature_val = 0.f;
        float prev_humidity_val    = 0.f;

        //Pyöritään tässä silmukassa
        while(true){



            sleep(1);

            /*
            //Uusi lämpötilarvo jaetussa muistissa
            if(prev_temperature_val != shared_mem->temperature){

                printf("p2 temp: %f\n", shared_mem->temperature);
                prev_temperature_val = shared_mem->temperature;
            }
            */
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