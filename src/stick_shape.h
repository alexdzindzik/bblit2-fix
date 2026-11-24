// Analógico do controle XInput -> analógico do controle do PS1 (contas puras, testadas no Linux)
#ifndef STICK_SHAPE_H
#define STICK_SHAPE_H
// x, y de -1 a 1 (cima = +y). Zona morta radial de dead % do curso; logo depois dela a saída já vale start %
// (o primeiro toque anda devagar) e chega ao máximo em outer % do curso (nem todo analógico chega ao fim em todas
// as direções). Por fim o círculo vira quadrado: a diagonal toda dá o máximo nos dois eixos, como no PS1.
void stick_shape(double x, double y, int dead, int start, int outer, double *ox, double *oy);
// -1..1 -> 0..255 (centro 127), o valor de um eixo no pacote do controle do PS1
unsigned char stick_ps1(double v);
#endif
