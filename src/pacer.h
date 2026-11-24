// Horários dos quadros extras dentro de um tick do jogo (60 fps, etapa 1). Puro, em unidades de QPC.
#ifndef PACER_H
#define PACER_H

// QPC por quadro: freq / fps, arredondado.
long long pacer_period(long long freq, int fps);
// Próximo quadro extra: o primeiro ponto da grade g0 + m·period que é > after e cai em
// [start + period/2, end − max(margin, period/2)). O quadro do próprio jogo (na troca de tick) ocupa o lugar do
// ponto da grade mais próximo dele, por isso meio período de folga nas pontas; assim a média fica em fps.
// Devolve −1 se não houver.
long long pacer_next(long long g0, long long period, long long start, long long after, long long end, long long margin);

#endif
