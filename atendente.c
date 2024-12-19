#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>


#define MAX_QUEUE_SIZE 100

pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;

// Estrutura para representar um cliente
typedef struct {
    int pid;
    int prioridade;
    long long hora_chegada;
    int paciencia;
    int tempo_atend;
} Cliente;

// Fila de clientes
Cliente fila[MAX_QUEUE_SIZE];
int inicio_fila = 0, fim_fila = 0, tamanho_fila = 0; 
double clientes_satisfeitos = 0; 
double clientes_insatisfeitos = 0;
int stop = 0;

sem_t *sem_atend, *sem_block; //semáforos

sem_t *sem_fila; //semáforo de fila
sem_t *sem_fila_cheia; //semáforo se a fila estiver cheia

void* parar(void* arg){
    char c;
	while((c = getchar()) != EOF){
        if(c == 's' || c == 'S'){
            //sem_close(sem_atend);
            stop = 1;     
            break;
        }
        break;
    }
    return NULL;

}

long long current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Adiciona um cliente na fila
void adicionar_cliente(Cliente cliente) {
    sem_wait(sem_fila);
    if (tamanho_fila < MAX_QUEUE_SIZE) {
        fila[fim_fila] = cliente;
        fim_fila = (fim_fila + 1) % MAX_QUEUE_SIZE;
        tamanho_fila++;
    }
    sem_post(sem_fila);

}

void criar_cliente(void *arg){

    int X = *(int *)arg;
    Cliente cliente;
    cliente.pid = fork();

    if(cliente.pid == 0){

        //printf("Criando processo cliente...\n");
        execl("./client", "client", NULL);
        exit(0);

    }else if (cliente.pid > 0){

        cliente.hora_chegada = current_time_ms();
        cliente.prioridade = rand() % 2;
        cliente.paciencia = cliente.prioridade ? X / 2 : X;

        // Espera o cliente dormir
        waitpid(cliente.pid, NULL, WUNTRACED);

        // Lê o tempo de atendimento do cliente
        FILE *demanda = fopen("demanda.txt", "r");
        fscanf(demanda, "%d", &cliente.tempo_atend);
        fclose(demanda);

        // Adiciona o cliente na fila
        adicionar_cliente(cliente);
        /*printf("[Recepção] Cliente %d adicionado com prioridade %d, paciência %d ms, e tempo de atendimento %d µs\n",
                cliente.pid, cliente.prioridade, cliente.paciencia, cliente.tempo_atend);*/

    }else printf("Cliente não pôde ser criado!");


}

// Thread Recepção
void *recepcao(void *arg) {

    int *args = (int *)arg; // Cast the argument
    int N = args[0];
    int X = args[1];
    
    //Criação de Semáforos
    sem_atend = sem_open("/sem_atend", O_CREAT, 0644, 1);
    sem_block = sem_open("/sem_block", O_CREAT, 0644, 1);

    sem_fila = sem_open("/sem_fila", O_CREAT, 0644, 1);
    sem_fila_cheia = sem_open("/sem_fila_cheia", O_CREAT, 0644, 1);

    //Cria thread para parar execução do loop infinito
    if(N == 0){

        pthread_t thread;
        pthread_create(&thread, NULL, parar, NULL);

    }

    //Cria N ou infinitos processos Cliente
    for (int i = 0; (N == 0 || i < N) && !stop; i++) {

        if (tamanho_fila < MAX_QUEUE_SIZE) {
            criar_cliente(&X);
        } else sem_wait(sem_fila_cheia);
        //printf("Tamanho da Fila: %d", tamanho_fila);

    }

    return NULL;
}

// Remove um cliente da fila
Cliente remover_cliente() {
    sem_wait(sem_fila);
    Cliente cliente = fila[inicio_fila];
    inicio_fila = (inicio_fila + 1) % MAX_QUEUE_SIZE;
    tamanho_fila--;
    sem_post(sem_fila);

    int fila_status;
    sem_getvalue(sem_fila_cheia, &fila_status);
    if(fila_status == 0) sem_post(sem_fila_cheia);

    return cliente;
}

char get_process_state(pid_t pid) {
    char path[64];
    char state;
    FILE *stat_file;

    // Construct the path to the process stat file
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    // Open the stat file
    stat_file = fopen(path, "r");
    if (stat_file == NULL) {
        perror("Failed to open /proc/<pid>/stat");
        return '\0'; // Return '\0' if the process doesn't exist
    }

    // Read the state from the file
    fscanf(stat_file, "%*d %*s %c", &state);
    fclose(stat_file);

    return state; // Return the process state
}

int check_if_asleep(pid_t pid) {

    char state = get_process_state(pid);
    if (state == 'T') {
        return 1;
    } else {
        return 0;
    }

}

void acordar_analista() {

    FILE *pid_analista = fopen("analista_pid.txt", "r");

    int analista_pid = 0;
    fscanf(pid_analista, "%d", &analista_pid);
    fclose(pid_analista);

    while(!check_if_asleep(analista_pid));

    kill(analista_pid, SIGCONT);
}


void add_to_lng(Cliente cliente, Cliente fila_lng[], int i){
    i = i%10;
    fila_lng[i] = cliente;
    if((i + 1)%10 == 0){
        FILE *lng = fopen("lng.txt", "a");
        for(int j = 0; j < 10; j++){
            cliente = fila_lng[j];
            fprintf(lng, "%d\n", cliente.pid);
            //printf("Cliente %d adicionado ao arquivo lng.txt\n", cliente.pid);
        }
        fclose(lng);
    }
    
}

void *atendente(void *arg) {

    int *args = (int *)arg; // Cast the argument
    int N = args[0];
    int X = args[1];

    Cliente fila_lng[10];

    printf("Iniciando thread do atendente...\n");

    for(int i = 0; (N == 0 || i < N) && !stop; i++) {
        //printf("Rodei!\n");
        while(tamanho_fila <= 0);
        if (1) {
            //printf("Atendendo cliente...\n");
            // Pega o cliente da fila e remove    
            //usleep(1000);    
            Cliente cliente = remover_cliente();
            //printf("Cliente PID: %d\n", cliente.pid);
            // Acorda o cliente
            //printf("Acordando cliente %d\n", cliente.pid);
            int sem_atend_val = 0;
            sem_getvalue(sem_atend, &sem_atend_val);
            //printf("\nFirst sem_atend = %d\n\n", sem_atend_val);
            if(sem_atend_val == 0) sem_post(sem_atend);
            kill(cliente.pid, SIGCONT);
            
            //sem_getvalue(sem_atend, &sem_atend_val);
            //printf("\nSecond sem_atend = %d\n\n", sem_atend_val);

            // Espera sem_atend abrir
            //printf("Esperando semáforo sem_atend abrir...\n");
            sem_wait(sem_atend);
            //printf("Semáforo sem_atend aberto\n");
            
            // Espera o semáforo abrir para continuar. Antes de entrar na região crítica, fecha o semáforo
            sem_wait(sem_block);

            // ***Início da região crítica***,
            // Escreve pid no arquivo lng.txt
            /*
            FILE *lng = fopen("lng.txt", "a");
            fprintf(lng, "%d\n", cliente.pid);
            fclose(lng);
            printf("Cliente %d adicionado ao arquivo lng.txt\n", cliente.pid);
            */

            add_to_lng(cliente, fila_lng, i);

            // Abre semaforo sem_block
            sem_post(sem_block);
            //sem_post(sem_atend);

            // Calcula satisfacao do cliente
            long long tempo_atual = current_time_ms();
            int tempo_espera = tempo_atual - cliente.hora_chegada;
            //printf("Tempo de espera: %d ms\n", tempo_espera);
            //printf("Tempo atual: %lld\n", tempo_atual);
            // Se o cliente estiver satisfeito, incrementa o contador
            if (tempo_espera <= cliente.paciencia) {
                //printf("Cliente satisfeito\n");
                clientes_satisfeitos++;
            }else{
                clientes_insatisfeitos++;
                //printf("Cliente insatisfeito\n");
            } 
            //printf("!((i+1) mod 10) = %d\n", !((i+1)%10));
            if(!((i+1)%10)) acordar_analista(); //acorda o analista a cada 10 iterações
            //printf("Atendente: i = %d\n", i);
            
            //waitpid(cliente.pid, NULL, 0);
        };
    };

    acordar_analista(); //acordar uma última vez para garantir o arquivo estar limpo (vai com certeza ter 10 ou menos elementos nessa chamada)

    return NULL;
}

int main(int argc, char *argv[]) {

    int N = atoi(argv[1]);
    int X = atoi(argv[2]);

    long long inicio = current_time_ms();

    sem_unlink("/sem_atend");
    sem_unlink("/sem_block");

    int args[2] = {N, X}; //define argumentos para recepção e atendente
    pthread_t thread_recepcao, thread_atendente, thread_parar;
    pthread_create(&thread_recepcao, NULL, recepcao, args);

    pid_t analista_pid = fork();
    if (analista_pid == 0) {
        //printf("Executando analista...\n");
        execl("./analista", "analista", NULL);
        perror("Erro ao executar o analista");
        exit(1);
    }

    waitpid(analista_pid, NULL, WUNTRACED);

    pthread_create(&thread_atendente, NULL, atendente, args);
    
    pthread_join(thread_recepcao, NULL);
    //printf("Recepção finalizada\n");
    pthread_join(thread_atendente, NULL);
    //printf("Atendimento finalizado\n");

    if (N == 0) {
        pthread_create(&thread_parar, NULL, parar, NULL);
        //pthread_join(thread_parar, NULL);
        //printf("N == 0?");
    }

    //printf("I'm about to be timed!\n");
    long long fim = current_time_ms();
    //printf("Current Time GET\n");

    acordar_analista();
    kill(analista_pid, SIGTERM);
    waitpid(analista_pid, NULL, 0);
    //printf("Analyst KILLED!");

    sem_close(sem_atend);
    //printf("sem_atend CLOSED!\n");
    sem_close(sem_block);
    //printf("sem_block CLOSED!\n");
    sem_unlink("/sem_atend");
    //printf("sem_atend UNLINKED!\n");
    sem_unlink("/sem_block");
    //printf("sem_block UNLINKED!\n");

    double taxa_satisfacao = (double)clientes_satisfeitos / (clientes_satisfeitos + clientes_insatisfeitos);
    printf("Taxa de satisfação: %.2f%%\n", taxa_satisfacao * 100);
    printf("Tempo total de execução: %lld ms\n", fim - inicio);

    return 0;   
}