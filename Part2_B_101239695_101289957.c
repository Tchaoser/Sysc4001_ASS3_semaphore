#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>

#define NUM_STUDENTS 20
#define NUM_CYCLES 3
#define NUM_TAS 5

// Structure for shared memory
struct shared_data {
    sem_t sem[NUM_TAS];
    char students[NUM_STUDENTS][5]; // 20 students, each 4 chars + '\0'
};

int main() {
    srand(time(NULL));

    // Create shared memory for semaphores and student list
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_SHARED | MAP_ANONYMOUS;
    struct shared_data* shm = (struct shared_data*) mmap(NULL, sizeof(struct shared_data), prot, flags, -1, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Initialize semaphores
    for (int i = 0; i < NUM_TAS; i++) {
        if (sem_init(&shm->sem[i], 1, 1) < 0) {
            perror("sem_init");
            return 1;
        }
    }

    // Read the database file into shared memory
    FILE* fin = fopen("database.txt", "r");
    if (!fin) {
        perror("fopen database.txt");
        return 1;
    }

    for (int i = 0; i < NUM_STUDENTS; i++) {
        if (fscanf(fin, "%4s", shm->students[i]) != 1) {
            fprintf(stderr, "Error reading line %d from database.txt\n", i+1);
            fclose(fin);
            return 1;
        }
    }
    fclose(fin);

    // Fork 5 TAs
    for (int ta_id = 1; ta_id <= NUM_TAS; ta_id++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
            // Child: TA process
            // Each TA processes the students from the start and does 3 cycles
            int cycle_count = 0;
            int current_index = 0;

            while (cycle_count < NUM_CYCLES) {
                // ACCESS DATABASE PHASE
                // TA j locks sem j and sem (j+1)%5
                // zero-based indexing for semaphores: TA ta_id => sem index (ta_id-1)
                int left_sem = ta_id - 1;
                int right_sem = (ta_id % NUM_TAS);

                // Wait (lock) on both semaphores
                sem_wait(&shm->sem[left_sem]);
                sem_wait(&shm->sem[right_sem]);

                // Now access the database
                printf("TA %d is accessing the database.\n", ta_id);
                fflush(stdout);

                // Simulate database access delay 1-4 sec
                int delay_db = 1 + rand() % 4;
                sleep(delay_db);

                // Get the current student from shared memory
                const char* student_id = shm->students[current_index];

                // Release semaphores so others can access the database
                sem_post(&shm->sem[right_sem]);
                sem_post(&shm->sem[left_sem]);

                // Now MARKING PHASE
                printf("TA %d is marking student %s.\n", ta_id, student_id);
                fflush(stdout);

                // Check if we reached "9999" -> end of cycle
                if (strcmp(student_id, "9999") == 0) {
                    cycle_count++;
                    current_index = 0; 
                } else {
                    current_index++;
                    if (current_index >= NUM_STUDENTS) {
                        // Should not happen, but in case:
                        current_index = 0;
                    }
                }

                // Random mark between 0 and 10
                int mark = rand() % 11;
                // Simulate marking delay 1-10 sec
                int delay_mark = 1 + rand() % 10;
                sleep(delay_mark);

                // Just print the mark since we are not writing to file now
                printf("TA %d finished marking student %s with mark %d.\n", ta_id, student_id, mark);
                fflush(stdout);
            }

            printf("TA %d finished all cycles.\n", ta_id);
            fflush(stdout);
            _exit(0);
        }
        // Parent continues to spawn the other TAs
    }

    // Parent: wait for all TAs
    for (int i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

    // Destroy semaphores and unmap memory
    for (int i = 0; i < NUM_TAS; i++) {
        sem_destroy(&shm->sem[i]);
    }

    munmap(shm, sizeof(*shm));

    printf("All TAs have finished.\n");
    return 0;
}
