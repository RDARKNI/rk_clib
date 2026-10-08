struct A {
  int        x;
  static int xx;
} a;
int test() { return A::xx; }
