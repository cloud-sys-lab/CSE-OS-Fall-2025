#include <stdio.h>
#include <stdlib.h>
#include "page_replacement.h"


// Fill frame array with -1, when its empty
void init_frames(int *frame, int frames) {
    int i;
    for (i = 0; i < frames; i++) {
        frame[i] = -1;
    }
}

// Search for a page in the frame array. Returns index if found, -1 is returned if not found
int find_page(const int *frame, int frames, int page) {
    int i;
    for (i = 0; i < frames; i++) {
        if (frame[i] == page) {
            return i;
        }
    }
    return -1;
}

// FIFO replacement
SimResult simulate_fifo(const int *refs, int n, int frames) {
    SimResult res;
    int *frame;
    int next;
    int i;

    res.hits = 0;
    res.faults = 0;

    frame = (int *)malloc(frames * sizeof(int));
    if (frame == NULL) {
        return res;
    }

    init_frames(frame, frames);
    next = 0;

    for (i = 0; i < n; i++) {
        int page = refs[i];
        int pos = find_page(frame, frames, page);

        if (pos != -1) {
            res.hits++;
        } else {
            res.faults++;
            frame[next] = page;
            next = (next + 1) % frames;
        }
    }

    free(frame);
    return res;
}

// Random replacement
SimResult simulate_random(const int *refs, int n, int frames) {
    SimResult res;
    int *frame;
    int i;

    res.hits = 0;
    res.faults = 0;

    frame = (int *)malloc(frames * sizeof(int));
    if (frame == NULL) {
        return res;
    }

    init_frames(frame, frames);

    for (i = 0; i < n; i++) {
        int page = refs[i];
        int pos = find_page(frame, frames, page);

        if (pos != -1) {
            res.hits++;
        } else {
            int free_index = -1;
            int j;

            res.faults++;

            for (j = 0; j < frames; j++) {
                if (frame[j] == -1) {
                    free_index = j;
                    break;
                }
            }

            if (free_index != -1) {
                frame[free_index] = page;
            } else {
                int evict_index = rand() % frames;
                frame[evict_index] = page;
            }
        }
    }

    free(frame);
    return res;
}

// LRU replacement
SimResult simulate_lru(const int *refs, int n, int frames) {
    SimResult res;
    int *frame;
    int *last_used;
    int i, t;

    res.hits = 0;
    res.faults = 0;

    frame = (int *)malloc(frames * sizeof(int));
    last_used = (int *)malloc(frames * sizeof(int));
    if (frame == NULL || last_used == NULL) {
        free(frame);
        free(last_used);
        return res;
    }

    init_frames(frame, frames);
    for (i = 0; i < frames; i++) {
        last_used[i] = -1;
    }

    for (t = 0; t < n; t++) {
        int page = refs[t];
        int pos = find_page(frame, frames, page);

        if (pos != -1) {
            res.hits++;
            last_used[pos] = t;
        } else {
            int free_index = -1;

            res.faults++;

            for (i = 0; i < frames; i++) {
                if (frame[i] == -1) {
                    free_index = i;
                    break;
                }
            }

            if (free_index != -1) {
                frame[free_index] = page;
                last_used[free_index] = t;
            } else {
                int lru_index = 0;
                int lru_time = last_used[0];

                for (i = 1; i < frames; i++) {
                    if (last_used[i] < lru_time) {
                        lru_time = last_used[i];
                        lru_index = i;
                    }
                }

                frame[lru_index] = page;
                last_used[lru_index] = t;
            }
        }
    }

    free(frame);
    free(last_used);
    return res;
}

// MIN (Optimal) replacement
SimResult simulate_min(const int *refs, int n, int frames) {
    SimResult res;
    int *frame;
    int t;

    res.hits = 0;
    res.faults = 0;

    frame = (int *)malloc(frames * sizeof(int));
    if (frame == NULL) {
        return res;
    }

    init_frames(frame, frames);

    for (t = 0; t < n; t++) {
        int page = refs[t];
        int pos = find_page(frame, frames, page);

        if (pos != -1) {
            res.hits++;
        } else {
            int free_index = -1;
            int i;

            res.faults++;

            for (i = 0; i < frames; i++) {
                if (frame[i] == -1) {
                    free_index = i;
                    break;
                }
            }

            if (free_index != -1) {
                frame[free_index] = page;
            } else {
                int evict_index = -1;
                int farthest = -1;

                for (i = 0; i < frames; i++) {
                    int page_in_frame = frame[i];
                    int j;
                    int next_use = -1;

                    for (j = t + 1; j < n; j++) {
                        if (refs[j] == page_in_frame) {
                            next_use = j;
                            break;
                        }
                    }

                    if (next_use == -1) {
                        evict_index = i;
                        farthest = n + 1;
                        break;
                    }

                    if (next_use > farthest) {
                        farthest = next_use;
                        evict_index = i;
                    }
                }

                if (evict_index < 0) {
                    evict_index = 0;
                }

                frame[evict_index] = page;
            }
        }
    }

    free(frame);
    return res;
}

// Print header for trace tables
void print_trace_header(int frames) {
    int i;

    printf("Time | Ref |");
    for (i = 0; i < frames; i++) {
        printf(" F%d ", i);
    }
    printf("| H/F\n");

    printf("-----+-----+");
    for (i = 0; i < frames; i++) {
        printf("----");
    }
    printf("+----\n");
}

// FIFO trace
void fifo_trace(const int *refs, int n, int frames) {
    int *frame;
    int next;
    int t, i;

    frame = (int *)malloc(frames * sizeof(int));
    if (frame == NULL) {
        return;
    }

    init_frames(frame, frames);
    next = 0;

    print_trace_header(frames);

    for (t = 0; t < n; t++) {
        int page = refs[t];
        int pos = find_page(frame, frames, page);
        char status;

        if (pos != -1) {
            status = 'H';
        } else {
            status = 'F';
            frame[next] = page;
            next = (next + 1) % frames;
        }

        printf("%4d | %3d |", t, page);
        for (i = 0; i < frames; i++) {
            if (frame[i] == -1) {
                printf("  - ");
            } else {
                printf("%3d ", frame[i]);
            }
        }
        printf("| %c\n", status);
    }

    free(frame);
}

// Random trace
void random_trace(const int *refs, int n, int frames) {
    int *frame;
    int t, i;

    frame = (int *)malloc(frames * sizeof(int));
    if (frame == NULL) {
        return;
    }

    init_frames(frame, frames);

    print_trace_header(frames);

    for (t = 0; t < n; t++) {
        int page = refs[t];
        int pos = find_page(frame, frames, page);
        char status;

        if (pos != -1) {
            status = 'H';
        } else {
            int free_index = -1;
            int j;

            status = 'F';

            for (j = 0; j < frames; j++) {
                if (frame[j] == -1) {
                    free_index = j;
                    break;
                }
            }

            if (free_index != -1) {
                frame[free_index] = page;
            } else {
                int evict_index = rand() % frames;
                frame[evict_index] = page;
            }
        }

        printf("%4d | %3d |", t, page);
        for (i = 0; i < frames; i++) {
            if (frame[i] == -1) {
                printf("  - ");
            } else {
                printf("%3d ", frame[i]);
            }
        }
        printf("| %c\n", status);
    }

    free(frame);
}

// LRU trace
void lru_trace(const int *refs, int n, int frames) {
    int *frame;
    int *last_used;
    int t, i;

    frame = (int *)malloc(frames * sizeof(int));
    last_used = (int *)malloc(frames * sizeof(int));
    if (frame == NULL || last_used == NULL) {
        free(frame);
        free(last_used);
        return;
    }

    init_frames(frame, frames);
    for (i = 0; i < frames; i++) {
        last_used[i] = -1;
    }

    print_trace_header(frames);

    for (t = 0; t < n; t++) {
        int page = refs[t];
        int pos = find_page(frame, frames, page);
        char status;

        if (pos != -1) {
            status = 'H';
            last_used[pos] = t;
        } else {
            int free_index = -1;

            status = 'F';

            for (i = 0; i < frames; i++) {
                if (frame[i] == -1) {
                    free_index = i;
                    break;
                }
            }

            if (free_index != -1) {
                frame[free_index] = page;
                last_used[free_index] = t;
            } else {
                int lru_index = 0;
                int lru_time = last_used[0];

                for (i = 1; i < frames; i++) {
                    if (last_used[i] < lru_time) {
                        lru_time = last_used[i];
                        lru_index = i;
                    }
                }

                frame[lru_index] = page;
                last_used[lru_index] = t;
            }
        }

        printf("%4d | %3d |", t, page);
        for (i = 0; i < frames; i++) {
            if (frame[i] == -1) {
                printf("  - ");
            } else {
                printf("%3d ", frame[i]);
            }
        }
        printf("| %c\n", status);
    }

    free(frame);
    free(last_used);
}

// MIN (Optimal) trace
void min_trace(const int *refs, int n, int frames) {
    int *frame;
    int t, i;

    frame = (int *)malloc(frames * sizeof(int));
    if (frame == NULL) {
        return;
    }

    init_frames(frame, frames);

    print_trace_header(frames);

    for (t = 0; t < n; t++) {
        int page = refs[t];
        int pos = find_page(frame, frames, page);
        char status;

        if (pos != -1) {
            status = 'H';
        } else {
            int free_index = -1;

            status = 'F';

            for (i = 0; i < frames; i++) {
                if (frame[i] == -1) {
                    free_index = i;
                    break;
                }
            }

            if (free_index != -1) {
                frame[free_index] = page;
            } else {
                int evict_index = -1;
                int farthest = -1;

                for (i = 0; i < frames; i++) {
                    int page_in_frame = frame[i];
                    int j;
                    int next_use = -1;

                    for (j = t + 1; j < n; j++) {
                        if (refs[j] == page_in_frame) {
                            next_use = j;
                            break;
                        }
                    }

                    if (next_use == -1) {
                        evict_index = i;
                        farthest = n + 1;
                        break;
                    }

                    if (next_use > farthest) {
                        farthest = next_use;
                        evict_index = i;
                    }
                }

                if (evict_index < 0) {
                    evict_index = 0;
                }

                frame[evict_index] = page;
            }
        }

        printf("%4d | %3d |", t, page);
        for (i = 0; i < frames; i++) {
            if (frame[i] == -1) {
                printf("  - ");
            } else {
                printf("%3d ", frame[i]);
            }
        }
        printf("| %c\n", status);
    }

    free(frame);
}
