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

// Shared memory structure
struct shared_data {
    sem_t sem[NUM_TAS];
    char students[NUM_STUDENTS][5]; // 20 lines of 4-digit numbers, last is "9999"
};

int main() {
    srand(time(NULL));

    // Create shared memory
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_SHARED | MAP_ANONYMOUS;
    struct shared_data* shm = (struct shared_data*) mmap(NULL, sizeof(struct shared_data), prot, flags, -1, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Initialize semaphores in shared memory
    for (int i = 0; i < NUM_TAS; i++) {
        if (sem_init(&shm->sem[i], 1, 1) < 0) {
            perror("sem_init");
            return 1;
        }
    }

    // Load the database into shared memory
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
            // Child process: TA logic
            char filename[20];
            snprintf(filename, sizeof(filename), "TA%d.txt", ta_id);
            FILE* fout = fopen(filename, "w");
            if (!fout) {
                fprintf(stderr, "TA %d: Could not open output file.\n", ta_id);
                _exit(1);
            }

            int cycle_count = 0;
            int current_index = 0;

            // Determine semaphore order to avoid circular wait:
            int left_sem = ta_id - 1;
            int right_sem = (ta_id % NUM_TAS);

            int first_sem = (left_sem < right_sem) ? left_sem : right_sem;
            int second_sem = (left_sem < right_sem) ? right_sem : left_sem;

            while (cycle_count < NUM_CYCLES) {
                // Acquire semaphores in a global order
                sem_wait(&shm->sem[first_sem]);
                sem_wait(&shm->sem[second_sem]);

                // Now access the database
                printf("TA %d is accessing the database.\n", ta_id);
                fflush(stdout);

                int delay_db = 1 + rand() % 4;
                sleep(delay_db);

                // Get the current student from shared memory
                const char* student_id = shm->students[current_index];

                // Release semaphores after reading the student
                sem_post(&shm->sem[second_sem]);
                sem_post(&shm->sem[first_sem]);

                // Marking phase
                printf("TA %d is marking student %s.\n", ta_id, student_id);
                fflush(stdout);

                // Check if we reached "9999"
                if (strcmp(student_id, "9999") == 0) {
                    cycle_count++;
                    current_index = 0; 
                } else {
                    current_index++;
                    if (current_index >= NUM_STUDENTS) {
                        current_index = 0;
                    }
                }

                // Random mark between 0 and 10
                int mark = rand() % 11;
                // Marking delay 1-10 sec
                int delay_mark = 1 + rand() % 10;
                sleep(delay_mark);

                // Write to TA file
                fprintf(fout, "%s %d\n", student_id, mark);
                fflush(fout);

                printf("TA %d finished marking student %s with mark %d.\n", ta_id, student_id, mark);
                fflush(stdout);
            }

            fclose(fout);
            printf("TA %d finished all cycles.\n", ta_id);
            fflush(stdout);
            _exit(0);
        }
        // Parent continues to next TA
    }

    // Parent waits for all TAs
    for (int i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

    // Destroy semaphores and unmap shared memory
    for (int i = 0; i < NUM_TAS; i++) {
        sem_destroy(&shm->sem[i]);
    }
    munmap(shm, sizeof(*shm));

    printf("All TAs have finished.\n");
    return 0;
}
