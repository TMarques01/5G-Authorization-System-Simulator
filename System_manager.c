#include "shared_memory.h"

// Function to write in log file
void write_log(char *writing){
    
    // Sempahore for writing in the file
    sem_wait(log_semaphore);

    // Variables for time
    now = time(NULL);
    t = localtime(&now);

    fprintf(log_file, "%d-%02d-%02d %02d:%02d:%02d  %s\n",
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec, writing);

    sem_post(log_semaphore);
}
 
//Função para verificar se uma string é um número
int is_number(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0; // Não é um número
        }
    }
    return 1; // É um número
}

//Função de verificação do ficheiro
int file_verification() {
    FILE *f = fopen("config.txt", "r");

    if (f == NULL) {
        printf("Erro ao abrir o ficheiro\n");
        return -1;
    }


    char line[50];
    int count = 0;
    int temp_val;

    config = malloc(sizeof(program_init));

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';

        // Verifica se a linha contém um número
        if (is_number(line)) {
            temp_val = atoi(line);
            // Verifica se o valor está dentro dos critérios esperados
            if (temp_val < 0 || ((count == 2 || count == 4 || count == 5) && temp_val < 1)) {
                printf("Valores incorretos no ficheiro\n");
                fclose(f);
                return -2;
            }

            // Atribui o valor lido à estrutura, baseado no contador
            switch (count) {
                case 0: config->max_mobile_users = temp_val; break;
                case 1: config->queue_pos = temp_val; break;
                case 2: config->auth_servers = temp_val; break;
                case 3: config->auth_proc_time = temp_val; break;
                case 4: config->max_video_wait = temp_val; break;
                case 5: config->max_others_wait = temp_val; break;
                default:
                    fclose(f);
                    return -3; // Número inesperado de linhas
            }
            count++;
        } else {
            printf("Linha não é um número válido.\n");
            fclose(f);
            return -2;
        }
    }

    fclose(f);

    if (count != 6) {
        printf("Número insuficiente de dados!\n");
        return -3;
    }

    return 0; // Sucesso
}

// Monitor engine process function
void monitor_engine(){
    sleep(1);
}

// Receiver funcion
void *receiver(void *arg){
    write_log("THREAD RECEIVER CREATED");
    sleep(1);
    pthread_exit(NULL);
}

// Sender function
void *sender(void *arg){
    write_log("THREAD SENDER CREATED");
    sleep(1);
    pthread_exit(NULL);
}

// Authorization request manager function
void authorization_request_manager(){

    // Argumento para já será 0
    if (pthread_create(&receiver_thread, NULL,sender, 0) != 0) {
        printf("CANNOT CREATE RECEIVER_THREAD\n");
        exit(1);
    }

    // Argumento para já será 1
    if (pthread_create(&sender_thread, NULL,receiver,(void*) 1) != 0) {
        printf("CANNOT CREATE SENDER_THREAD\n");
        exit(1);
    }

    // Closing threads
    pthread_cancel(receiver_thread);
    pthread_cancel(sender_thread);
}

// Closing function
void cleanup(){

    write_log("5G_AUTH_PLATFORM SIMULATOR WAITING FOR LAST TASKS TO FINISH");
    // Wait for Authorization and Monitor engine
    for(int i=0;i<2;i++){
		wait(NULL);
	}

    write_log("5G_AUTH_PLATFORM SIMULATOR CLOSING");

    // Close log file and destroy semaphores
    if (sem_close(log_semaphore) == -1) printf("ERROR CLOSING LOG SEMAPHORE\n");
    if (sem_unlink(LOG_SEM_NAME) == -1 ) printf ("ERROR UNLINKING LOG SEMAPHORE\n");
    if (fclose(log_file) == EOF) printf("ERROR CLOSIGN LOG FILE\n");

    //Free config malloc
    free(config);

    
}

// Function to initialize log file and log semaphore
void init_log(){

    //Delete semaphore if is open
    sem_unlink(LOG_SEM_NAME);
    log_file = fopen("log.txt", "w");
    log_semaphore = sem_open(LOG_SEM_NAME, O_CREAT | O_EXCL, 0777, 1);
    if (log_semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    } 


}

int main(int argc, char* argv[]){

    // Verify "config" file
    if (file_verification() != 0){
        return 0;
    }

    // Inicialize log file and log sempahore
    init_log();

    // Isto está a escrever bué vezes no ficheiro não sei porquê
    write_log("5G_AUTH_PLATFORM SIMULATOR STARTING");

    // Create monitor_engine_process
    monitor_engine_process = fork();
    if (monitor_engine_process == 0){

        // Writing for log file
        write_log("PROCESS MONITOR ENGINE CREATED");
        monitor_engine();
        exit(0);
    } else if (monitor_engine_process == -1){
        perror("CANNOT CREAT MONITOR ENGINE PROCESS\n");
        exit(1);
    }

    // Create authorization requeste manager process
    authorization_request_manager_process = fork();
    if (authorization_request_manager_process == 0){

        // Writing for log file
        write_log("PROCESS AUTHORIZATION REQUEST MANAGER CREATED");
        authorization_request_manager();
        exit(0);
    } else if (authorization_request_manager_process == -1){
        perror("CANNOT CREAT AUTHORIZATION REQUESTE MANAGER PROCESS\n");
        exit(1);
    }

    //Cleaning...
    cleanup();

    return 0;
}