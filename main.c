#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

typedef struct {  // задание для 1 потока
    int *in;
    int *out;
    int rows;
    int cols;
    int w;
    int r0;
    int r1;
    int demo;
} Task;

static void sort_window(int *arr, int len) {  // сортировка окна для нахождения медианы
    for (int i = 1; i < len; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// основная функция, медианный фильтр по строкам от r0 до r1.
static void median_region(int *in, int *out, int rows, int cols, int w, int r0, int r1) {
    int half = w / 2;
    int win_area = w * w;
    int *window = (int *)malloc(win_area * sizeof(int));
    for (int i = r0; i < r1; i++) {
        for (int j = 0; j < cols; j++) {
            int count = 0;
            for (int di = -half; di <= half; di++) {  // собираем соседей
                int ii = i + di;
                if (ii < 0) ii = 0; // обработка краев
                if (ii >= rows) ii = rows - 1;
                for (int dj = -half; dj <= half; dj++) {
                    int jj = j + dj;
                    if (jj < 0) jj = 0;
                    if (jj >= cols) jj = cols - 1;

                    window[count++] = in[ii * cols + jj];
                }
            }
            sort_window(window, win_area);
            out[i * cols + j] = window[win_area / 2];  // медиана
        }
    }
    free(window);
}

static void *worker(void *arg) {  //точка входа потока
    Task *t = (Task *)arg;
    if (t->demo) {  // если демо режим -d то поток пишет свой ID и диапазон строк
        printf("[thread] id=%lu rows=[%d, %d)\n",
               (unsigned long)pthread_self(), t->r0, t->r1);
        fflush(stdout); // принудительно пишем текст на экран чтобы строки не перемешались
    }
    median_region(t->in, t->out, t->rows, t->cols, t->w, t->r0, t->r1);  // солнце светит рабочий пашет
    return NULL;
}

// один шаг фильтрации в nthreads потоков, форк энд джоин
static void run_parallel_step(int *in, int *out, int rows, int cols, int w,
                              int nthreads, int demo,
                              pthread_t *threads, Task *tasks) {
    int base = rows / nthreads;  // сколько строк точно достанется каждому потоку
    int rem  = rows % nthreads;  // остаток строк
    int start = 0;               // начальная строка для очередного потока

    // форк - раздаем задания и запускаем потоки
    for (int k = 0; k < nthreads; k++) {
        // если есть остаток то первым rem потокам даем на 1 строку больше
        int len = base + (k < rem ? 1 : 0);

        tasks[k].in   = in;
        tasks[k].out  = out;
        tasks[k].rows = rows;
        tasks[k].cols = cols;
        tasks[k].w    = w;
        tasks[k].r0   = start;
        tasks[k].r1   = start + len;
        tasks[k].demo = demo;
        start += len;
        // системный вызов создания потока - поток сразу начинает выполнять worker
        pthread_create(&threads[k], NULL, worker, &tasks[k]);
    }

    // джоин - главный поток ждёт пока остальные закончат работу
    for (int k = 0; k < nthreads; k++) {
        pthread_join(threads[k], NULL);
    }
}

// получение текущего времени
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// последовательная версия, работает в 1 главном потоке
static double algorithm_sequential(int *a, int *b, int rows, int cols, int w, int k) {
    double t0 = now_ms(); // засекаем время

    // применяем фильтр k раз
    for (int i = 0; i < k; i++) {
        median_region(a, b, rows, cols, w, 0, rows);
        // меняем местами a и b
        int *tmp = a; a = b; b = tmp;
    }
    return now_ms() - t0; // возвращаем затраченное время
}

// параллельная версия, работает в nthreads потоков
static double algorithm_parallel(int *a, int *b, int rows, int cols, int w, int k, int nthreads, int demo) {
    // выделяем массивы для хранения идентификаторов потоков и их заданий
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * nthreads);
    Task *tasks = (Task *)malloc(sizeof(Task) * nthreads);

    double t0 = now_ms(); // засекаем время

    // применяем фильтр k раз
    for (int i = 0; i < k; i++) {
        run_parallel_step(a, b, rows, cols, w, nthreads, demo, threads, tasks);
        // меняем местами a и b
        int *tmp = a; a = b; b = tmp;
    }

    double dt = now_ms() - t0; // считаем затраченное время

    free(threads);
    free(tasks);
    return dt;
}

int main(int argc, char **argv) {
    // параметры по умолчанию
    int rows = 1000, cols = 1000, w = 3, k = 3, nthreads = 4, reps = 3, demo = 0;

    // разбираем командную строку
    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-n") == 0 && i + 1 < argc) rows = cols = atoi(argv[++i]);
        else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) w = atoi(argv[++i]);
        else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) k = atoi(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) nthreads = atoi(argv[++i]);
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) reps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-d") == 0) demo = 1;
    }

    // размер окна медианного фильтра должен быть нечетным
    if (w % 2 == 0) w++;
    if (w < 1) w = 1;
    if (nthreads < 1) nthreads = 1;
    if (reps < 1) reps = 1;
    size_t sz = (size_t)rows * cols; // общее количество элементов в матрице

    // выделяем память под матрицы: a b для последовательного алгоритма, a2 b2 для параллельного
    int *a  = (int *)malloc(sz * sizeof(int));
    int *b  = (int *)malloc(sz * sizeof(int));
    int *a2 = (int *)malloc(sz * sizeof(int));
    int *b2 = (int *)malloc(sz * sizeof(int));

    // заполняем матрицу
    srand(12345);
    for (size_t i = 0; i < sz; i++) {
        a[i] = rand() % 256;
    }

    // демонстрация работы потоков (1 итерация)
    if (demo) {
        memcpy(a2, a, sz * sizeof(int));
        printf("DEMO (threads=%d)\n", nthreads);
        algorithm_parallel(a2, b2, rows, cols, w, 1, nthreads, 1);
        printf("DONE\n");
        free(a); free(b); free(a2); free(b2);
        return 0;
    }

    // копия исходной матрицы т к k итераций перезапишут буферы
    int *orig = (int *)malloc(sz * sizeof(int));
    memcpy(orig, a, sz * sizeof(int));

    // замеряем последовательный алгоритм
    double ts_min = 1e300;
    for (int r = 0; r < reps; r++) {
        memcpy(a, orig, sz * sizeof(int)); // восстанавливаем исходные данные перед замером
        double t = algorithm_sequential(a, b, rows, cols, w, k);
        if (t < ts_min) ts_min = t;
    }

    // замеряем параллельный
    double tp_min = 1e300;
    for (int r = 0; r < reps; r++) {
        memcpy(a2, orig, sz * sizeof(int)); // восстанавливаем исходные данные
        double t = algorithm_parallel(a2, b2, rows, cols, w, k, nthreads, 0);
        if (t < tp_min) tp_min = t;
    }

    // ищем в какой матрице лежит ркзультат
    int *res_seq = (k % 2 == 1) ? b : a;
    int *res_par = (k % 2 == 1) ? b2 : a2;

    // проверяем что параллельный код посчитал всё так же как последовательный
    int ok = 1;
    for (size_t i = 0; i < sz; i++) {
        if (res_seq[i] != res_par[i]) {
            ok = 0;
            break;
        }
    }

    // метрики производительности
    double speedup    = ts_min / tp_min;       // ускорение
    double efficiency = speedup / nthreads;    // эффективность

    // отсчёт
    printf("size=%dx%d w=%d k=%d threads=%d reps=%d\n", rows, cols, w, k, nthreads, reps);
    printf("sequential_ms=%.3f\n", ts_min);
    printf("parallel_ms=%.3f\n", tp_min);
    printf("speedup=%.4f\n", speedup);
    printf("efficiency=%.4f\n", efficiency);
    printf("match=%s\n", ok ? "yes" : "no");

    // освобождаем память
    free(a); free(b); free(a2); free(b2); free(orig);
    return 0;
}