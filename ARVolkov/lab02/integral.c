#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
 
/* Подынтегральная функция- вариант D */
static double f(double x) {
    return (x * x - 1.0) * pow(10.0, -2.0 * x);
}
 
/* Точное значение интеграла - для оценки погрешности
 * k = 2 ln 10, первообразная */
static double exact(double a, double b) {
    const double k = 2.0 * log(10.0);
    double Fa = exp(-k * a) * (1.0 / k - a * a / k - 2.0 * a / (k * k) - 2.0 / (k * k * k));
    double Fb = exp(-k * b) * (1.0 / k - b * b / k - 2.0 * b / (k * k) - 2.0 / (k * k * k));
    return Fb - Fa;
}
 
/* Последовательный метод средних прямоугольников */
static double integral_seq(double a, double b, long n) {
    double h = (b - a) / n;
    double sum = 0.0;
    for (long i = 0; i < n; i++) {
        sum += f(a + (i + 0.5) * h);
    }
    return sum * h;
}
 
/* Параллельный метод средних прямоугольников - OpenMP + reduction*/
static double integral_par(double a, double b, long n) {
    double h = (b - a) / n;
    double sum = 0.0;
    #pragma omp parallel for reduction(+:sum)
    for (long i = 0; i < n; i++) {
        sum += f(a + (i + 0.5) * h);
    }
    return sum * h;
}
 
/* Вычисление с заданной точностью: удваиваем n до |I_2n - I_n| < eps.
 * par = 0 - последовательная версия, par = 1 - параллельная.
 * В *n_out возвращается найденное число разбиений. */
static double integral_eps(double a, double b, long n0, double eps, int par, long *n_out) {
    long n = n0;
    double prev = par ? integral_par(a, b, n) : integral_seq(a, b, n);
    while (1) {
        n *= 2;
        double cur = par ? integral_par(a, b, n) : integral_seq(a, b, n);
        if (fabs(cur - prev) < eps) {
            *n_out = n;
            return cur;
        }
        prev = cur;
    }
}
 
int main(int argc, char **argv) {
    double a = (argc > 1) ? atof(argv[1]) : 0.0;
    double b = (argc > 2) ? atof(argv[2]) : 1.0;
    long n = (argc > 3) ? atol(argv[3]) : 100000000L;
    int threads = (argc > 4) ? atoi(argv[4]) : 8;
    double eps = (argc > 5) ? atof(argv[5]) : 0.0;
 
    if (n <= 0 || threads <= 0 || b <= a) {
        fprintf(stderr, "Использование: %s [a] [b] [n] [потоки] [eps]\n", argv[0]);
        return 1;
    }
 
    omp_set_num_threads(threads);
    double ref = exact(a, b);
 
    printf("f(x) = (x^2 - 1) * 10^(-2x), отрезок [%g; %g]\n", a, b);
    printf("Точное значение  : %.12f\n\n", ref);
 
    if (eps > 0.0) {
        /*режим заданной точности */
        long n_seq, n_par;
 
        double t = omp_get_wtime();
        double I_seq = integral_eps(a, b, n, eps, 0, &n_seq);
        double t_seq = omp_get_wtime() - t;
 
        t = omp_get_wtime();
        double I_par = integral_eps(a, b, n, eps, 1, &n_par);
        double t_par = omp_get_wtime() - t;
 
        printf("Режим заданной точности: eps = %g, начальное n = %ld\n", eps, n);
        printf("Последовательная : n = %ld, I = %.12f, погрешность = %.3e, время = %.6f с\n",
               n_seq, I_seq, fabs(I_seq - ref), t_seq);
        printf("Параллельная     : n = %ld, I = %.12f, погрешность = %.3e, время = %.6f с, потоков = %d\n",
               n_par, I_par, fabs(I_par - ref), t_par, threads);
        printf("Ускорение        : %.2f\n", t_seq / t_par);
        return 0;
    }
 
    /* фиксированное n*/
    double t = omp_get_wtime();
    double I_seq = integral_seq(a, b, n);
    double t_seq = omp_get_wtime() - t;
 
    t = omp_get_wtime();
    double I_par = integral_par(a, b, n);
    double t_par = omp_get_wtime() - t;
 
    printf("n = %ld\n", n);
    printf("Последовательная : I = %.12f, погрешность = %.3e, время = %.6f с, потоков = 1\n",
           I_seq, fabs(I_seq - ref), t_seq);
    printf("Параллельная     : I = %.12f, погрешность = %.3e, время = %.6f с, потоков = %d\n",
           I_par, fabs(I_par - ref), t_par, threads);
    printf("Ускорение        : %.2f\n", t_seq / t_par);
    return 0;
}