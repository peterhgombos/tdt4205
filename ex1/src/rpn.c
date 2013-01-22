#include "rpn.h"
#include <stdlib.h>

RpnCalc newRpnCalc(){
    RpnCalc calc = (RpnCalc*)malloc(sizeof(RpnCalc));
    calc.size = 64;
    calc.top = 0;
    calc.stack = (double*)malloc(calc.size*sizeof(double));
    return calc;
}

void push(RpnCalc* rpnCalc, double n){
    if(rpnCalc.top == rpnCalc.size){
        rpnCalc.size *= 2;
        rpnCalc.stack = (double*)realloc(rpnCalc.stack, rpnCalc.size*sizeof(double));
    }
    rpnCalc[++rpnCalc.top] = n;
}

void performOp(RpnCalc* rpnCalc, char op){
    double num1 = rpnCalc.stack[rpnCalc.top--];
    double num2 = rpnCalc.stack[rpnCalc.top--];
    double result;
    switch(op){
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
        case '/': result = num1 / num2; break;
    }
    push(rpnCalc, result);
}

double peek(RpnCalc* rpnCalc){
    return rpnCalc.stack[rpnCalc.top];
}
