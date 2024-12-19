#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>

#define MAX_PIDS 10
#define LNG_FILE "lng.txt"
#define ANALISTA_PID_FILE "analista_pid.txt"

sem_t *sem_block;

void processar_lng();

int main() {

    // Criar arquivo com o PID do analista
    printf("Criando arquivo com o PID do analista\n");
    FILE *pid_file = fopen(ANALISTA_PID_FILE, "w");
    if (pid_file == NULL) {
        perror("Erro ao criar arquivo de PID do analista");
        exit(1);
    }

    //printf("Escrevendo PID no arquivo\n");
    fprintf(pid_file, "%d", getpid());
    fclose(pid_file);
    
    sem_block = sem_open("/sem_block", O_RDWR, 0644, 1);
    
    if (sem_block == SEM_FAILED) {
        perror("Erro ao abrir o semáforo");
        exit(1);
    }
    
    printf("PID Analista: %d\n", getpid());

    while(1) {
        //printf("Analista entrando em modo de espera...\n");
        
        raise(SIGSTOP);

        //printf("Analista acordado!\n");

        // Bloquear acesso ao arquivo LNG
        //printf("Bloqueando acesso ao arquivo LNG\n");
        sem_wait(sem_block);

        // Processar arquivo LNG
        //printf("Processando arquivo LNG\n");
        processar_lng();
        
        // Liberar acesso ao arquivo LNG
        //printf("Liberando acesso ao arquivo LNG\n");
        sem_post(sem_block);

    }

    sem_close(sem_block);

    return 0;
}


void processar_lng() {
    FILE *lng_file = fopen(LNG_FILE, "r+");
    if (lng_file == NULL) {
        perror("Erro ao abrir o arquivo LNG");
        return;
    }

    int pids[MAX_PIDS];
    int count = 0;
    
    // lendo 10 PIDs
    while (count < MAX_PIDS && fscanf(lng_file, "%d", &pids[count]) == 1) {
        count++;
    }

    // imprimindo 10 PIDs
    printf("PIDs processados pelo analista:\n");
    for (int i = 0; i < count; i++) {
        printf("%d ", pids[i]);
    }
    printf("\n");

    // Posicionar o ponteiro após os 10 primeiros PIDs
    long remaining_start = ftell(lng_file);

    // Ler o restante do arquivo
    char buffer[4096];
    fseek(lng_file, remaining_start, SEEK_SET);
    size_t bytes_read = fread(buffer, 1, sizeof(buffer), lng_file);

    // Reescrever o arquivo com os PIDs restantes
    fseek(lng_file, 0, SEEK_SET);
    if (bytes_read > 0) {
        fwrite(buffer, 1, bytes_read, lng_file);
    }

    // Truncar o arquivo para remover o espaço extra
    ftruncate(fileno(lng_file), ftell(lng_file));

    fclose(lng_file);
    //printf("PIDs lidos foram removidos e o arquivo foi reorganizado.\n");

    // // Remover os PIDs lidos do arquivo
    // printf("Removendo PIDs lidos do arquivo...\n");
    // fseek(lng_file, 0, SEEK_SET);
    // char buffer[1024];
    // size_t bytes_read;
    // while ((bytes_read = fread(buffer, 1, sizeof(buffer), lng_file)) > 0) {
    //     fseek(lng_file, -bytes_read, SEEK_CUR);
    //     fwrite(buffer, 1, bytes_read, lng_file);
    // }
    // ftruncate(fileno(lng_file), ftell(lng_file));

    // fclose(lng_file);
};