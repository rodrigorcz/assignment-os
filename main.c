#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define resta_estoque (materia_deposito || materia_fabrica)
#define canetas_disponiveis (canetas_enviadas <= 0) || (canetas_deposito > 0) || (canetas_recebidas>0)

// entrada
int estoque_materia_prima = 10;
int tempo_materia_entrega = 1;
int tempo_fabricacao_caneta = 1;
int tempo_solicitacao_comprador = 6;
int canetas_compradadas_por_solicitacao = 3;
int capacidade_deposito_canetas = 3;
int materia_enviada_iteracao = 1;

// buffers
int materia_deposito;
int materia_fabrica = 0;
int canetas_deposito = 0;
int canetas_solicitadas = 0;
int canetas_enviadas = 0;
int canetas_recebidas = -1;
int qnt_disponivel = 0;

// semaforo
int controle_bloqueia = 0;

// mutexes
pthread_mutex_t mutex_materia_deposito = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_materia_fabrica = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_canetas_deposito = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_canetas_recebidas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_canetas_enviadas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_qnt_disponivel = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_canetas_solicitadas = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_controle_bloqueia = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t deposito_canetas_cheio = PTHREAD_COND_INITIALIZER;

void* deposito_materia() {
    time_t tempo_inicial = time(NULL);

    while(materia_deposito) {

        if((int) (time(NULL) - tempo_inicial) >= tempo_materia_entrega) {

            while(controle_bloqueia != 0)
                pthread_cond_wait(&deposito_canetas_cheio, &mutex_controle_bloqueia);
            pthread_mutex_unlock(&mutex_controle_bloqueia);

            materia_deposito -= materia_enviada_iteracao;
            materia_fabrica += materia_enviada_iteracao;

            tempo_inicial = time(NULL);
        }
    }

    pthread_exit(NULL);
}

void *fabrica_canetas() {
    time_t tempo_inicial;

    while(1) {

        int materia_local = materia_fabrica;
       

        if(materia_local) {

            tempo_inicial = time(NULL);
            while(time(NULL) - tempo_inicial < tempo_fabricacao_caneta);

         
            while(controle_bloqueia != 0)
                pthread_cond_wait(&deposito_canetas_cheio, &mutex_controle_bloqueia);

            materia_fabrica--;
            canetas_deposito++;
        }
    }

    pthread_exit(NULL);
}

void *deposito_canetas() {
    
    while(1) {
        if(canetas_solicitadas != 0){

            if(canetas_deposito >= canetas_solicitadas)
                canetas_enviadas = canetas_solicitadas;
            else
                canetas_enviadas = canetas_deposito;
            
            canetas_deposito -= canetas_enviadas;

            canetas_solicitadas = 0;
        }
        qnt_disponivel = capacidade_deposito_canetas - canetas_deposito;
    }

    pthread_exit(NULL);
}

void *comprador() {
    time_t tempo_inicial;

    while(1){

        canetas_solicitadas = canetas_compradadas_por_solicitacao;

        tempo_inicial = time(NULL);
        while(time(NULL) - tempo_inicial < tempo_fabricacao_caneta);

        canetas_recebidas = canetas_enviadas;
        canetas_enviadas = 0;
    
    }

}

void *controle() {
    while(1){   

        while(qnt_disponivel == 0)
            controle_bloqueia = 1;

        pthread_cond_broadcast(&deposito_canetas_cheio);

        controle_bloqueia = 0;
    }
}

int criador() {

    materia_deposito = estoque_materia_prima;
    // criação das threads
    pthread_t t_deposito_materia;
    pthread_t t_fabrica_caneta;
    pthread_t t_controle;
    pthread_t t_deposito_caneta;
    pthread_t t_comprador;

    if (pthread_create(&t_deposito_materia, NULL, (void*) deposito_materia, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 1;
    }

    if (pthread_create(&t_fabrica_caneta, NULL, (void*) fabrica_canetas, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 1;
    }

    if (pthread_create(&t_controle, NULL, (void*) controle, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 1;
    }

    if (pthread_create(&t_deposito_caneta, NULL, (void*) deposito_canetas, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 1;
    }

    if (pthread_create(&t_comprador, NULL, (void*) comprador, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 1;
    }

    while(resta_estoque || canetas_disponiveis){

        pthread_mutex_lock(&mutex_canetas_recebidas);

        if(canetas_recebidas != -1){
            printf("Canetas Compradas %d \n", canetas_recebidas);
            canetas_recebidas = -1;
        }
        pthread_mutex_unlock(&mutex_canetas_recebidas);
    }

    return 0;
}

int main(void) {
    
    criador();

    return 0;
}