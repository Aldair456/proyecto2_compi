int main() {
    int num1 = 100;
    int num2 = 50;
    int num3 = 25;
    int num4 = 10;
    int num5 = 5;

    int sum = num1 + num2 + num3 + num4 + num5;
    int product = num1 * 2;
    int division = num1 / num5;
    int subtraction = num1 - num2;

    int intermediate1 = sum + product;
    int intermediate2 = division * subtraction;
    int intermediate3 = intermediate1 - intermediate2;

    int counter = 0;
    for(int i = 0; i < 10; i++) {
        counter = counter + i;
    }

    int factorial = 1;
    for(int j = 1; j <= 5; j++) {
        factorial = factorial * j;
    }

    int result = intermediate3 + counter + factorial;

    return result;
}