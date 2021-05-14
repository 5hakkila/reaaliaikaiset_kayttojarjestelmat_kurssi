#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <mysql.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include "dht22data.h"

//Jaetun muistin tai FIFON(kesken) valinta
#define USE_SHARED_MEM  true
#define USE_FIFO        !USE_SHARED_MEM

//Random-arvojen generointi
bool read_data(dht22Data *data);

int main(int argc, char** argv) {

#if(USE_FIFO)

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

#elif(USE_SHARED_MEM)

    //Dht22Data-tyyppinen osoitin jaettuun muistiin
     dht22Data *shared_mem = (dht22Data*) mmap(NULL,
                            sizeof(dht22Data),
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS,
                            -1,
                            0); 

#endif

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

#if (USE_FIFO)
        //Avataan fifo kirjoittamista varten
        int fd = open("sensoridata", O_WRONLY);

        //Tarkistaminen virheiden varalta
        if(fd == -1){
            printf("Ei voitu avata fifoa");
            exit(-1);
        }

#endif
        //Luetaan anturidataa sekunnin välein
        while(true){

            //1s sleep
            sleep(1);
        
            //Luetaan uudet arvot
            if(read_data(&dht22_data)){

#if (USE_FIFO)      
                char tmpTemperature[20];

                //Muutetaan merkkijonoksi
                sprintf(tmpTemperature, "%f ", dht22_data.temperature);

                //Kirjoitetaan uusi arvo FIFOON
                write(fd, tmpTemperature, sizeof(tmpTemperature));

#elif(USE_SHARED_MEM)
                //Uudet arvot jaettuun muistiin
                memcpy(shared_mem, &dht22_data, sizeof(dht22_data));
#endif
                //printf("p1: temperature: %f\n", dht22_data.temperature);
                //printf("p1: humidity: %f\n", dht22_data.humidity);
            }
        }
#if(USE_FIFO)
        //Suljetaan FIFO
        close(fd);
#endif
    }

    //Ei-reaaliaikainen prosessi, datan lähettäminen pilveen
    else if(pid > 0){

        //Tietokantayhteys
        MYSQL *mysql_connection = mysql_init(NULL);

        //Tarkistetaan tietokantayhteyden alustaminen
        if(mysql_connection == NULL){
            printf("Tietokantaan yhdistäminen epäonnistui\n");
            exit(-1);
        }

        //Yhdistetään tietokantaan
        if (mysql_real_connect(mysql_connection,
             "localhost",
             "root", 
             "raspbianmysql",
             "dht22data",
             0, NULL, 0) == NULL){

            printf("Tunnistautuminen tietokantaan epäonnistui\n");
            mysql_close(mysql_connection);
            exit(1);
        }

#if (USE_FIFO)
        //Avataan FIFO anturidatan lukemista varten
        int fd = open("sensoridata", O_RDONLY);

        if(fd == -1){
            printf("Ei voitu avata fifoa\n");
            exit(-1);
        }
#endif
        //Edelliset arvot talteen
        float prev_temperature_val = 0.f;
        float prev_humidity_val    = 0.f;

        //Kirjoitetaan uusia mittausarvoja kantaan
        while(true){

            //1s sleep
            sleep(1);
#if (USE_FIFO)
             
#elif (USE_SHARED_MEM)

            //Uudet arvot jaetussa muistissa
            if(prev_temperature_val != shared_mem->temperature ||
                prev_humidity_val != shared_mem->humidity){

                //printf("p2 temp: %f\n", shared_mem->temperature);
                prev_temperature_val = shared_mem->temperature;

                //printf("p2 humid: %f\n", shared_mem->humidity);
                prev_humidity_val = shared_mem->humidity;

                //Mysql statement
                char mysql_statement[150];

                snprintf(mysql_statement,
                    sizeof(mysql_statement),
                    "INSERT INTO data(temperature, humidity) VALUES(%.2f, %.2f)",
                    shared_mem->temperature,
                    shared_mem->humidity);

                //printf("statement: %s\n", mysql_statement);

                //Kirjoitetaan tietokantaan
                if(mysql_query(mysql_connection, mysql_statement)){
                    printf("Tietokantaan kirjoittaminen epäonnistui\n");
                    mysql_close(mysql_connection);
                 }
            }
#endif
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