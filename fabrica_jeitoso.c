#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define resta_estoque (materia_deposito || materia_fabrica || canetas_deposito)
#define minimo(x, y) ((x > y) ? y : x)

int estoque_materia_prima;
int materia_enviada_iteracao;
int tempo_materia_entrega;
int tempo_fabricacao_caneta;
int capacidade_desposito_canetas;
int canetas_compradadas_por_solicitacao;
int tempo_solicitacao_comprador;

int materia_deposito;
int espacos_disponiveis;
int materia_fabrica = 0;
int canetas_deposito = 0;
int canetas_solicitadas = 0;
int canetas_enviadas = 0;
char permissao_controle = '1';
char solicitacao_comprador = '1';

pthread_mutex_t mutex_materia_deposito = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_materia_fabrica = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_canetas_deposito = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_espacos_disponiveis = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_impressao_mensagem = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t permissao_controle_producao = PTHREAD_COND_INITIALIZER;
pthread_cond_t deposito_canetas_cheio = PTHREAD_COND_INITIALIZER;
pthread_cond_t deposito_canetas_livre = PTHREAD_COND_INITIALIZER;
pthread_cond_t solicitacao_compra_canetas = PTHREAD_COND_INITIALIZER;

void imprime_solicitacao_compra() {
    if(canetas_enviadas) {
        if(canetas_solicitadas) {
            printf("%d caneta(s) comprada(s), resta(m) %d.\n", canetas_enviadas, canetas_solicitadas);
        } else {
            printf("%d caneta(s) comprada(s), solicitação atendida.\n", canetas_enviadas);
        }
     } else {
        printf("Nao ha canetas em estoque no momento. Volte mais tarde!\n");
     }
}


void* deposito_materia(void *thread_args) {
    time_t tempo_inicial = time(NULL);

    while(materia_deposito) {
        
        if((int) ((time(NULL) - tempo_inicial)) >= tempo_materia_entrega) {
            pthread_mutex_lock(&mutex_materia_fabrica);

            while(permissao_controle == '0') {
                pthread_cond_wait(&permissao_controle_producao, &mutex_materia_fabrica);
            }

            pthread_mutex_lock(&mutex_materia_deposito);

            materia_fabrica += minimo(materia_enviada_iteracao, materia_deposito);
            materia_deposito -= minimo(materia_enviada_iteracao, materia_deposito);

            pthread_mutex_unlock(&mutex_materia_deposito);
            pthread_mutex_unlock(&mutex_materia_fabrica);

            tempo_inicial = time(NULL);
        }
    }

    pthread_exit(NULL);
}

void *fabrica_canetas(void *thread_args) {
    time_t tempo_inicial;

    while(materia_deposito || materia_fabrica) {
        if(materia_fabrica) {
            tempo_inicial = time(NULL);

            while(time(NULL) - tempo_inicial < tempo_fabricacao_caneta);

            pthread_mutex_lock(&mutex_materia_fabrica);

            while(permissao_controle == '0') {
                pthread_cond_wait(&permissao_controle_producao, &mutex_materia_fabrica);
            }

            pthread_mutex_lock(&mutex_canetas_deposito);

            materia_fabrica--;
            canetas_deposito++;

            pthread_mutex_unlock(&mutex_canetas_deposito);
            pthread_mutex_unlock(&mutex_materia_fabrica);
        }
    }

    pthread_exit(NULL);
}

void *controle(void *thread_args) {
    while(resta_estoque) {
        pthread_mutex_lock(&mutex_espacos_disponiveis);

        while(espacos_disponiveis != 0) {
            pthread_cond_wait(&deposito_canetas_livre, &mutex_espacos_disponiveis);
        }

        permissao_controle = '0';

        while(espacos_disponiveis == 0) {
            pthread_cond_wait(&deposito_canetas_cheio, &mutex_espacos_disponiveis);
        }

        permissao_controle = '1';

        pthread_cond_broadcast(&permissao_controle_producao);
        pthread_mutex_unlock(&mutex_espacos_disponiveis);
    }
}

void *deposito_canetas(void *thread_args) {
    while(resta_estoque) {
        pthread_mutex_lock(&mutex_espacos_disponiveis);

        espacos_disponiveis = capacidade_desposito_canetas - canetas_deposito;

        if(!espacos_disponiveis) {
            pthread_cond_broadcast(&deposito_canetas_livre);
        }

        pthread_mutex_unlock(&mutex_espacos_disponiveis);

        if(solicitacao_comprador == '2') {
            pthread_mutex_lock(&mutex_canetas_deposito);

            if(canetas_deposito >= canetas_solicitadas) {
                canetas_enviadas = canetas_solicitadas;
                canetas_deposito -= canetas_solicitadas;
                canetas_solicitadas = 0;
            }  else {
                canetas_enviadas = canetas_deposito;
                canetas_solicitadas -= canetas_deposito;
                canetas_deposito = 0;
            }

            pthread_mutex_unlock(&mutex_canetas_deposito);
            pthread_cond_broadcast(&deposito_canetas_cheio);

            solicitacao_comprador = '0';

            pthread_mutex_unlock(&mutex_impressao_mensagem);
            pthread_cond_broadcast(&solicitacao_compra_canetas);
        }
    }
}

void *comprador(void *thread_args) {
    time_t tempo_inicial = time(NULL);
    
    while(resta_estoque) {
        pthread_mutex_lock(&mutex_impressao_mensagem);

        while(solicitacao_comprador != '1') {
            pthread_cond_wait(&solicitacao_compra_canetas, &mutex_impressao_mensagem);
        }

        while(time(NULL) - tempo_inicial < tempo_solicitacao_comprador);

        if(!canetas_solicitadas) {
            canetas_solicitadas = canetas_compradadas_por_solicitacao;
        }

        solicitacao_comprador = '2';

        tempo_inicial = time(NULL);
    }

    pthread_exit(NULL);

}

int criador(void *thread_args) {
    pthread_t t_deposito_materia;
    pthread_t t_fabrica_canetas;
    pthread_t t_controle;
    pthread_t t_deposito_canetas;
    pthread_t t_comprador_canetas;

    if (pthread_create(&t_deposito_materia, NULL, (void*) deposito_materia, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 0;
    }

    if (pthread_create(&t_fabrica_canetas, NULL, (void*) fabrica_canetas, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 0;
    }

    if (pthread_create(&t_controle, NULL, (void*) controle, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 0;
    }

    if (pthread_create(&t_deposito_canetas, NULL, (void*) deposito_canetas, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 0;
    }

    if (pthread_create(&t_comprador_canetas, NULL, (void*) comprador, NULL) != 0){
        printf("Erro ao criar Thread! \n");
        return 0;
    }

    while(resta_estoque) {
        pthread_mutex_lock(&mutex_impressao_mensagem);

        while(solicitacao_comprador != '0') {
            pthread_cond_wait(&solicitacao_compra_canetas, &mutex_impressao_mensagem);
        }

        imprime_solicitacao_compra();

        solicitacao_comprador = '1';

        pthread_mutex_unlock(&mutex_impressao_mensagem);
        pthread_cond_broadcast(&solicitacao_compra_canetas);
    }

    return 1;
}

int main(void) {
    scanf("%d %d %d %d %d %d %d", &estoque_materia_prima, &materia_enviada_iteracao,
                                  &tempo_materia_entrega, &tempo_fabricacao_caneta,
                                  &capacidade_desposito_canetas, &canetas_compradadas_por_solicitacao,
                                  &tempo_solicitacao_comprador);

    materia_deposito = estoque_materia_prima;
    espacos_disponiveis = capacidade_desposito_canetas;

    if(criador(NULL) == 0) {
        printf("erro");
        return 0;
    }

    return 0;
}
