#include <forward_list>
#include <iostream>

using std::forward_list;

template<typename T>
void list_repacker(forward_list<T> &data) {

	forward_list<T>::iterator it = data.begin();
	forward_list<T> tmp_list;

	int size = std::distance(data.begin(), data.end());
	std::advance(it, size / 2);

	tmp_list.splice_after(tmp_list.before_begin(), data, it, data.end());
	tmp_list.reverse();

	it = data.begin();
	while (!tmp_list.empty()) {
		it = data.insert_after(it, tmp_list.front())++;
		tmp_list.pop_front();
	}
}

int main() {
	std::forward_list<int> second = { 1, 2, 3, 4, 5, 6, 7, 8};
	list_repacker(second);
	for(std::forward_list<int>::iterator it = second.begin(); it != second.end(); it++)
	{
		std::cout << *it << ' ';
	}
	return 0;
}