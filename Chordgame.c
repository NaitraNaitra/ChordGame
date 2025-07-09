#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <math.h>

#include <time.h>

#include <portaudio.h>

#include <ctype.h>

#include <fcntl.h>

#include <unistd.h>



#define SAMPLE_RATE 48000

#define BUFFER_SIZE 144000

#define NUM_NOTES 12

#define NUM_OCTAVES 9

#define MIN_OCTAVE 0

#define MAX_OCTAVE 8

#define MAX_SCALE_LENGTH 100

#define ANSI_COLOUR_RED     "\x1b[31m"

#define ANSI_COLOUR_GREEN     "\x1b[32m"

#define ANSI_COLOUR_RESET     "\x1b[0m"

#define ANSI_CLEAR_CONSOLE "\e[1;1H\e[2J"



const char* note_names[NUM_NOTES] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

const char* enharmonic_equivalents[NUM_NOTES] = {"C", "db", "D", "eb", "E", "F", "gb", "G", "ab", "A", "bb", "B"};

const char* valid_notes[] = {"C", "c", "C#", "c#", "Db", "db", "D", "d", "D#", "d#", "Eb", "eb", "E", "e", "F", "f", "F#", "f#", "Gb", "gb", "G", "g", "Ab", "ab", "A", "a", "A#", "a#", "Bb", "bb", "B", "b","r","R","s","S","q","Q"};

const int num_valid_notes = sizeof(valid_notes) / sizeof(valid_notes[0]);







// Normalize the note name (both to lowercase)

void normalize_note_name(const char* input, char* output) {

    int i = 0;

    while (input[i] != '\0') {

        output[i] = tolower((unsigned char)input[i]); // Convert everything to lowercase

        i++;

    }

    output[i] = '\0';

}



// Normalize input and find corresponding pitch class index (0–11), or -1 if invalid

int get_pitch_class_from_note(const char* note) {

    char normalized[4];

    normalize_note_name(note, normalized);



    for (int i = 0; i < NUM_NOTES; i++) {

        char name_norm[4], enh_norm[4];

        normalize_note_name(note_names[i], name_norm);

        normalize_note_name(enharmonic_equivalents[i], enh_norm);



        if (strcmp(normalized, name_norm) == 0 || strcmp(normalized, enh_norm) == 0) {

            return i;

        }

    }



    return -1;

}







int is_valid_note_input(const char *input) {

    return get_pitch_class_from_note(input) != -1;

}





double C0_frequency = 16.352;

int major_scale_intervals[7] = {2, 2, 1, 2, 2, 2, 1};



typedef struct {

    char name[3];

    int pitch_class;

    int octave;

    double frequency;

    const char* enharmonic_equiv;

} Note;







// Convert note name to lowercase, but preserve sharps

void normalize_note_name_for_enharmonic(const char* input, char* output) {

    int len = strlen(input);

    for (int i = 0; i < len; i++) {

        if (input[i] != '#') {

            output[i] = tolower((unsigned char)input[i]);

        } else {

            output[i] = input[i];

        }

    }

    output[len] = '\0';

}



// Function to compare note names (case-insensitive but preserve sharps)

int compare_note_names(const char* note1, const char* note2) {

    char normalized_note1[3], normalized_note2[3];

    normalize_note_name(note1, normalized_note1);

    normalize_note_name(note2, normalized_note2);

    return strcmp(normalized_note1, normalized_note2) == 0;

}



// Get the note number based on pitch class and octave

int get_note_number(int pitch_class, int octave) {

    return (octave + 1) * 12 + pitch_class;  // Shift octave correctly

}



// Get the frequency of the note based on pitch class and octave

double get_frequency(int pitch_class, int octave) {

    int note_number = get_note_number(pitch_class, octave);

    return C0_frequency * pow(2.0, (double)note_number / 12.0);

}



// Function to generate the major scale for a specific root note

void generate_major_scale(const char* root_note, Note* notes, int* num_notes, int range_low, int range_high, int allowed_pitch_classes[]) {

    int root_pitch_class = -1;

    for (int i = 0; i < NUM_NOTES; i++) {

        if (strcmp(root_note, note_names[i]) == 0 || strcmp(root_note, enharmonic_equivalents[i]) == 0) {

            root_pitch_class = i;

            break;

        }

    }

    if (root_pitch_class == -1) {

        printf("Error: Invalid root note %s\n", root_note);

        return;

    }



    //printf("Generating major scale from root note: %s (%d)\n", root_note, root_pitch_class);

    //printf("Octave range: %d to %d\n", range_low, range_high);



    int current_pitch_class = root_pitch_class;



    for (int octave = range_low; octave <= range_high; octave++) {

        current_pitch_class = root_pitch_class;  // reset for each octave

        for (int i = 0; i < 7; i++) {

            if (*num_notes >= MAX_SCALE_LENGTH) {

                //printf("Reached max scale length.\n");

                return;

            }



            if (allowed_pitch_classes[current_pitch_class]) {

                double freq = get_frequency(current_pitch_class, octave);

                /*printf("  Adding note %s (oct %d) | pitch_class: %d | freq: %.2f\n",

                       note_names[current_pitch_class], octave, current_pitch_class, freq); */



                notes[*num_notes] = (Note){

                    .pitch_class = current_pitch_class,

                    .octave = octave,

                    .frequency = freq,

                    .enharmonic_equiv = enharmonic_equivalents[current_pitch_class]

                };

                snprintf(notes[*num_notes].name, sizeof(notes[*num_notes].name), "%s", note_names[current_pitch_class]);

                (*num_notes)++;

            } else {

                printf("  Skipping pitch class %d (%s) not in allowed set\n", current_pitch_class, note_names[current_pitch_class]);

            }



            current_pitch_class = (current_pitch_class + major_scale_intervals[i]) % NUM_NOTES;

        }

    }

}



void get_allowed_pitch_classes(int allowed_pitch_classes[], const char* scales[], int num_scales) {

    memset(allowed_pitch_classes, 0, sizeof(int) * NUM_NOTES);



    for (int s = 0; s < num_scales; s++) {

        int root_pitch_class = -1;

        for (int i = 0; i < NUM_NOTES; i++) {

            if (strcmp(scales[s], note_names[i]) == 0 || strcmp(scales[s], enharmonic_equivalents[i]) == 0) {

                root_pitch_class = i;

                break;

            }

        }

        if (root_pitch_class == -1) {

            printf("Warning: Could not find pitch class for scale root '%s'\n", scales[s]);

            continue;

        }



        //printf("Building major scale for root: %s (pitch class %d)\n", scales[s], root_pitch_class);



        int current_pitch_class = root_pitch_class;

        for (int i = 0; i < 7; i++) {

            allowed_pitch_classes[current_pitch_class] = 1;

            //printf("  Allowing pitch class %d (%s)\n", current_pitch_class, note_names[current_pitch_class]);

            current_pitch_class = (current_pitch_class + major_scale_intervals[i]) % NUM_NOTES;

        }

    }

}



// Function to filter notes based on an octave range

void filter_notes_by_range(Note* notes, int* num_notes, int range_low, int range_high) {

    int filtered_count = 0;

    for (int i = 0; i < *num_notes; i++) {

        //printf("Filtering: Note %s (Octave %d)\n", notes[i].name, notes[i].octave);

        if (notes[i].octave >= range_low && notes[i].octave <= range_high) {

            //printf("  Keeping note %s octave %d\n", notes[i].name, notes[i].octave);

            notes[filtered_count++] = notes[i];

        } else {

            //printf("  Discarding note %s octave %d\n", notes[i].name, notes[i].octave);

        }

    }

    *num_notes = filtered_count;

}



// Compare function for sorting by frequency

int compare_by_frequency(const void* a, const void* b) {

    Note* note_a = (Note*)a;

    Note* note_b = (Note*)b;

    if (note_a->frequency < note_b->frequency) {

        return -1;

    } else if (note_a->frequency > note_b->frequency) {

        return 1;

    } else {

        return 0;

    }

}



// Function to remove duplicates from the list of notes

int remove_duplicates(Note* notes, int num_notes) {

    int unique_count = 0;

    for (int i = 0; i < num_notes; i++) {

        int is_duplicate = 0;

        for (int j = 0; j < unique_count; j++) {

            if (notes[i].pitch_class == notes[j].pitch_class && notes[i].octave == notes[j].octave) {

                is_duplicate = 1;

                break;

            }

        }

        if (!is_duplicate) {

            notes[unique_count++] = notes[i];

        }

    }

    return unique_count;

}



// Function to select random notes from the generated scale

void select_random_notes(Note* notes, int num_notes, int num_selected, Note* selected_notes) {

    static int seeded = 0;

    if (!seeded) {

        srand(time(NULL));

        seeded = 1;

    }



    //printf("\nSelecting unique pitch class notes from the full note pool...\n");



    // Copy the notes to an array we can shuffle

    Note available_notes[num_notes];

    for (int i = 0; i < num_notes; i++) {

        available_notes[i] = notes[i];

    }



    // Shuffle the entire available note pool

    for (int i = num_notes - 1; i > 0; i--) {

        int j = rand() % (i + 1);

        Note temp = available_notes[i];

        available_notes[i] = available_notes[j];

        available_notes[j] = temp;

    }



    // Now pick notes, ensuring unique pitch classes

    int pitch_class_used[NUM_NOTES] = {0};

    int selected_count = 0;



    for (int i = 0; i < num_notes && selected_count < num_selected; i++) {

        int pc = available_notes[i].pitch_class;

        if (!pitch_class_used[pc]) {

            selected_notes[selected_count++] = available_notes[i];

            pitch_class_used[pc] = 1;



            /*printf("  -> Selected: %s (Pitch class %d, Octave %d, Freq %.2f)\n",

                   available_notes[i].name, pc, available_notes[i].octave, available_notes[i].frequency); */

        }

    }



    if (selected_count < num_selected) {

        //printf("Warning: Not enough unique pitch classes available to select %d notes.\n", num_selected);

    }



    // Sort the selected notes by frequency (optional)

    qsort(selected_notes, selected_count, sizeof(Note), compare_by_frequency);



    /*printf("\nFinal selected notes (sorted by frequency):\n");

    for (int i = 0; i < selected_count; i++) {

        printf("  [%d] %s (Octave %d, Freq %.2f)\n", i + 1,

               selected_notes[i].name, selected_notes[i].octave, selected_notes[i].frequency);

    }*/

}

// Function to print the generated notes

void print_generated_scale(Note* generated_scale, int num_notes) {

    for (int i = 0; i < num_notes; i++) {

        printf(" Note: %-10s | Octave: %d | Frequency: %.2f Hz\n",

                generated_scale[i].name,

               generated_scale[i].octave, generated_scale[i].frequency);

    }

}



typedef struct {

    float buffer[BUFFER_SIZE];

    int index;

} AudioData;



static int audio_callback(const void* input, void* output, unsigned long framesPerBuffer,

                          const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {

    AudioData* data = (AudioData*)userData;

    float* out = (float*)output;

    for (unsigned int i = 0; i < framesPerBuffer; i++) {

        *out++ = data->buffer[data->index];

        data->index = (data->index + 1) % BUFFER_SIZE;

    }

    return paContinue;

}



void generate_wavetable(Note* selected_notes, int num_notes, AudioData* audioData) {

    memset(audioData->buffer, 0, sizeof(audioData->buffer));

    

    // Scale the amplitude of each note based on the number of notes

    double amplitude_factor = 1.0 / num_notes;



    for (int i = 0; i < num_notes; i++) {

        double frequency = selected_notes[i].frequency;



        for (int j = 0; j < BUFFER_SIZE; j++) {

            // Scale each note's contribution by the amplitude factor

            audioData->buffer[j] += amplitude_factor * sin(2.0 * M_PI * frequency * j / SAMPLE_RATE);

        }

    }

}



void play_audio(Note* selected_notes, int num_notes) {

    AudioData audioData = { .index = 0 };

    generate_wavetable(selected_notes, num_notes, &audioData);



    Pa_Initialize();

    PaStream* stream;

    Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, SAMPLE_RATE, 4096, audio_callback, &audioData);

    Pa_StartStream(stream);

    Pa_Sleep(3000);

    Pa_StopStream(stream);

    Pa_CloseStream(stream);

    Pa_Terminate();

}

void solo_audio(Note* selected_notes, int num_notes) {

    AudioData audioData = { .index = 0 };



    for (int i = 0; i < num_notes; i++) {

        

        // Generate the wavetable for a single note

        Note solo_note[1] = { selected_notes[i] };

        generate_wavetable(solo_note, 1, &audioData);



        // Play the single note

        Pa_Initialize();

        PaStream* stream;

        Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, SAMPLE_RATE, 4096, audio_callback, &audioData);

        Pa_StartStream(stream);

        Pa_Sleep(1000);  // Play each note for  a second

        int j = i + 1;

        printf("note [%d] is [%s] at %.2fHz\n",j , selected_notes[i].name, selected_notes[i].frequency );

        Pa_StopStream(stream);

        Pa_CloseStream(stream);

        Pa_Terminate();

    }

}



int is_enharmonic_match(const char *input, const char *target) {

    char input_normalized[3], target_normalized[3];

    normalize_note_name(input, input_normalized);

    normalize_note_name(target, target_normalized);



    // Direct match

    if (strcmp(input_normalized, target_normalized) == 0) {

        return 1;

    }



    // Check for known enharmonic equivalence pairs

    // Enharmonic pairs: F# <-> Gb, C# <-> Db, D# <-> Eb, A# <-> Bb

    if ((strcmp(input_normalized, "f#") == 0 && strcmp(target_normalized, "gb") == 0) ||

        (strcmp(input_normalized, "gb") == 0 && strcmp(target_normalized, "f#") == 0)) {

        return 1;

    }

    if ((strcmp(input_normalized, "c#") == 0 && strcmp(target_normalized, "db") == 0) ||

        (strcmp(input_normalized, "db") == 0 && strcmp(target_normalized, "c#") == 0)) {

        return 1;

    }

    if ((strcmp(input_normalized, "d#") == 0 && strcmp(target_normalized, "eb") == 0) ||

        (strcmp(input_normalized, "eb") == 0 && strcmp(target_normalized, "d#") == 0)) {

        return 1;

    }

    if ((strcmp(input_normalized, "a#") == 0 && strcmp(target_normalized, "bb") == 0) ||

        (strcmp(input_normalized, "bb") == 0 && strcmp(target_normalized, "a#") == 0)) {

        return 1;

    }



    return 0;

}



int compare_user_guess(Note* selected_notes, Note* user_guesses, int num_notes) {

    for (int i = 0; i < num_notes; i++) {

        int user_pc = get_pitch_class_from_note(user_guesses[i].name);

        int correct_pc = selected_notes[i].pitch_class;



        if (user_pc == -1 || user_pc != correct_pc) {

            return 0;  // Incorrect or invalid

        }

    }

    return 1;  // All matched

}



// --- Main Game Logic ---

int main(int argc, char* argv[]) {

    int total_correct = 0;

    int dev_null = open("/dev/null", O_WRONLY);

    if (dev_null != -1) {

        dup2(dev_null, STDERR_FILENO);  // Redirect stderr to /dev/null

        close(dev_null);

    }

    printf(ANSI_CLEAR_CONSOLE);

    if (argc < 5) {

        printf("Usage: %s -scale <scale> (C,E) -notes <numNotes> -range <low-high> -turns <turnCount>\n", argv[0]);

        return 1;

    }



    int num_notes = 0, num_turns = 0, range_low = -1, range_high = -1;

    Note generated_scale[MAX_SCALE_LENGTH];

    int num_generated_notes = 0;



    const char* scale_list[MAX_SCALE_LENGTH];

    int num_scales = 0;



    int allowed_pitch_classes[NUM_NOTES] = {0}; 



    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-scale") == 0) {

            char* token = strtok(argv[++i], ",");

            while (token != NULL && num_scales < MAX_SCALE_LENGTH) {

                scale_list[num_scales] = token;

                num_scales++;

                token = strtok(NULL, ",");

            }

        } else if (strcmp(argv[i], "-notes") == 0) {

            num_notes = atoi(argv[++i]);

        } else if (strcmp(argv[i], "-range") == 0) {

            if (sscanf(argv[++i], "%d-%d", &range_low, &range_high) != 2) {

                printf("Error: Invalid range format. Use <low-high>\n");

                return 1;

            }

        } else if (strcmp(argv[i], "-turns") == 0) {

            num_turns = atoi(argv[++i]);

        }

    }



    get_allowed_pitch_classes(allowed_pitch_classes, scale_list, num_scales);



    for (int i = 0; i < num_scales; i++) {

        generate_major_scale(scale_list[i], generated_scale, &num_generated_notes, range_low, range_high, allowed_pitch_classes);

    }



    qsort(generated_scale, num_generated_notes, sizeof(Note), compare_by_frequency);

    num_generated_notes = remove_duplicates(generated_scale, num_generated_notes);



    for (int turn = 0; turn < num_turns; turn++) {

        int total_turns = turn + 1;

        

        printf("\nTurn %d:\n", turn + 1);

        Note selected_notes[num_notes];

        select_random_notes(generated_scale, num_generated_notes, num_notes, selected_notes);

        

        printf("Playing audio...\n");

        play_audio(selected_notes, num_notes);



        Note user_guesses[num_notes];

        int correct_guesses = 0;



  /*      for (int i = 0; i < num_notes; i++) {

    char guess[4];

    while (1) {

        printf("Please guess note name [%d] (e.g., C, D#, Ab), or 'r' to repeat, 's' to solo, 'q' to quit: ", i + 1);

        char input_line[100];

        if (fgets(input_line, sizeof(input_line), stdin)) {

            sscanf(input_line, "%3s", guess);  // read up to 3 chars

        }



        // Check for commands BEFORE validating as note

        if (strcmp(guess, "R") == 0 || strcmp(guess, "r") == 0) {

            printf("Repeating selection.\n");

            play_audio(selected_notes, num_notes);

            continue;  // ask again for this note guess

        } 

        else if (strcmp(guess, "S") == 0 || strcmp(guess, "s") == 0) {

            printf(ANSI_CLEAR_CONSOLE);

            printf("Soloing selection.\n");

            solo_audio(selected_notes, num_notes);

            continue;  // ask again for this note guess

        } 

        else if (strcmp(guess, "Q") == 0 || strcmp(guess, "q") == 0) {

            printf("Quitting.\n");

            return 0;

        }



        // Now validate if the guess is a valid note

        if (!is_valid_note_input(guess)) {

            play_audio(selected_notes, num_notes);

            printf("Invalid note. Please enter a valid musical note.\n");

            continue;

        }



        // If valid note, accept and break to next note guess

        strcpy(user_guesses[i].name, guess);

        break;

    }

}

*/


        int i = 0;
while (i < num_notes) {
    char guess[4];
    printf("Please guess note name [%d] (e.g., C, D#, Ab), or 'r' to repeat, 's' to solo, 'x' to delete last, 'q' to quit: ", i + 1);
    char input_line[100];
    if (fgets(input_line, sizeof(input_line), stdin)) {
        sscanf(input_line, "%3s", guess);  // read up to 3 chars
    }

    if (strcmp(guess, "R") == 0 || strcmp(guess, "r") == 0) {
        printf("Repeating selection.\n");
        play_audio(selected_notes, num_notes);
        continue;
    } else if (strcmp(guess, "S") == 0 || strcmp(guess, "s") == 0) {
        printf(ANSI_CLEAR_CONSOLE);
        printf("Soloing selection.\n");
        solo_audio(selected_notes, num_notes);
        continue;
    } else if (strcmp(guess, "Q") == 0 || strcmp(guess, "q") == 0) {
        printf("Quitting.\n");
        return 0;
    } else if ((strcmp(guess, "X") == 0 || strcmp(guess, "x") == 0) && i > 0) {
        printf("Deleted last guess. Please re-enter.\n");
        i--;  // go back one guess
        continue;
    }

    // Validate input
    if (!is_valid_note_input(guess)) {
        play_audio(selected_notes, num_notes);
        printf("Invalid note. Please enter a valid musical note.\n");
        continue;
    }

    // Store valid guess
    strcpy(user_guesses[i].name, guess);
    i++;  // move to next guess
}

        if (compare_user_guess(selected_notes, user_guesses, num_notes)) {

            total_correct++;

            print_generated_scale(selected_notes, num_notes);

            Pa_Sleep(500);

            printf(ANSI_COLOUR_GREEN"Correct! You guessed all the notes correctly.\n"ANSI_COLOUR_RESET);

        } else {

            printf(ANSI_CLEAR_CONSOLE);

            solo_audio(selected_notes, num_notes);

            

            printf(ANSI_COLOUR_RED"Incorrect guesses. Better Luck Next Time.\n"ANSI_COLOUR_RESET);

        }

        Pa_Sleep(300);



        printf(ANSI_CLEAR_CONSOLE);

        float percentage = ((float) total_correct / total_turns) * 100;

        

            printf("You got %.2f%% of the guesses correct across all %d turns.\n", percentage, total_turns);



    }



    return 0;

}

