
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>


//how to run
//   gcc -pthread Part2_A_101239695_101289957.c -o Part2_A_101239695_101289957
//   ./Part2_A_101239695_101289957


#define NUM_TAS 5
#define NUM_ITERATIONS 3
#define STUDENT_LIST_FILE "database.txt"

sem_t semaphores[NUM_TAS];
int current_line = 0;
int iterations_completed = 0;


typedef struct {
    int ta_id;
    int current_iterations;
} TA_Args;

// Function to generate random delay
void random_delay(int min, int max) {
    int delay = rand() % (max - min + 1) + min;
    sleep(delay);
}

// Function to read next student from database
int get_next_student(FILE* db_file) {
    int student_number;
    
    // If end of file or reached end marker, reset
    if (fscanf(db_file, "%d", &student_number) != 1 || student_number == 9999) {
        rewind(db_file);
        fscanf(db_file, "%d", &student_number);
    }
    
    return student_number;
}


void* ta_process(void* arg) {
    TA_Args* ta_args = (TA_Args*)arg;
    int ta_id = ta_args->ta_id;
    
    // Open database file
    FILE* db_file = fopen(STUDENT_LIST_FILE, "r");
    if (!db_file) {
        perror("Error opening student list");
        return NULL;
    }
    
    // Create individual TA marking file
    char filename[20];
    snprintf(filename, sizeof(filename), "TA%d.txt", ta_id);
    FILE* ta_file = fopen(filename, "w");
    if (!ta_file) {
        perror("Error creating TA marking file");
        fclose(db_file);
        return NULL;
    }
    
    while (ta_args->current_iterations < NUM_ITERATIONS) {
        // Lock required semaphores (current and next)
        int next_sem = (ta_id % NUM_TAS);
        printf("TA %d trying to access database\n", ta_id);
        
        sem_wait(&semaphores[ta_id - 1]);
        sem_wait(&semaphores[next_sem]);
        
        // Access database and get next student
        int student_number = get_next_student(db_file);
        printf("TA %d selected student %d\n", ta_id, student_number);
        
        // Release semaphores
        sem_post(&semaphores[next_sem]);
        sem_post(&semaphores[ta_id - 1]);
        
        // Simulate marking process
        printf("TA %d marking student %d\n", ta_id, student_number);
        random_delay(1, 4);
        
        // Generate and save mark
        int mark = rand() % 11;
        fprintf(ta_file, "Student %d: Mark %d\n", student_number, mark);
        fflush(ta_file);
        
        // Simulate marking time
        random_delay(1, 10);
        
        // Check if completed an iteration
        if (student_number == 9999) {
            ta_args->current_iterations++;
        }
    }
    
    fclose(db_file);
    fclose(ta_file);
    return NULL;
}

// Function to create student list file
void create_student_list() {
    FILE* file = fopen(STUDENT_LIST_FILE, "w");
    if (!file) {
        perror("Error creating student list");
        exit(1);
    }
    
    // Generate 20 lines with 4 students per line
    for (int i = 0; i < 5; i++) {
        fprintf(file, "%04d %04d %04d %04d\n", 
                1 + i*4, 2 + i*4, 3 + i*4, 9999);
    }
    
    fclose(file);
}

int main() {
    // Seed random number generator
    srand(time(NULL));
    
    // Create student list file
    create_student_list();
    
    // Initialize semaphores
    for (int i = 0; i < NUM_TAS; i++) {
        sem_init(&semaphores[i], 0, 1);
    }
    
    // Create threads for TAs
    pthread_t ta_threads[NUM_TAS];
    TA_Args ta_args[NUM_TAS];
    
    for (int i = 0; i < NUM_TAS; i++) {
        ta_args[i].ta_id = i + 1;
        ta_args[i].current_iterations = 0;
        
        if (pthread_create(&ta_threads[i], NULL, ta_process, &ta_args[i]) != 0) {
            perror("Error creating TA thread");
            exit(1);
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_TAS; i++) {
        pthread_join(ta_threads[i], NULL);
    }
    
    // Destroy semaphores
    for (int i = 0; i < NUM_TAS; i++) {
        sem_destroy(&semaphores[i]);
    }
    
    return 0;
}
