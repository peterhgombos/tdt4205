#include "rpn.h"
#include <stdlib.h>

RpnCalc newRpnCalc(){
    return (RpnCalc){ 64, 0, malloc(64*sizeof(double))};
}

void push(RpnCalc* rpnCalc, double n){
    if(rpnCalc->top == rpnCalc->size){
        rpnCalc->size *= 2;
        rpnCalc->stack = realloc(rpnCalc->stack, rpnCalc->size*sizeof(double));
    }
    rpnCalc->stack[++rpnCalc->top] = n;
}

void performOp(RpnCalc* rpnCalc, char op){
    double num1 = rpnCalc->stack[rpnCalc->top--];
    double num2 = rpnCalc->stack[rpnCalc->top--];
    double result;
    switch(op){
        case '+': result = num2 + num1; break;
        case '-': result = num2 - num1; break;
        case '*': result = num2 * num1; break;
        case '/': result = num2 / num1; break;
    }
    push(rpnCalc, result);
}

double peek(RpnCalc* rpnCalc){
    return rpnCalc->stack[rpnCalc->top];
}
