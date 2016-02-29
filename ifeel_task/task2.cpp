#include <iostream>
using std::cout;

int main() {

	int N, M;
	std::cin >> N >> M;

	int part_size = N / M;
	int shift = N % M / 2;
	for (int i = 0; i < M; i++) {
		cout << part_size * i + shift << ' ' << part_size * (i + 1) + shift - 1 << std::endl;
	}
	return 0;
}