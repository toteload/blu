#pragma once

template<typename T> struct Vector {
  Allocator alloc;
  usize _len;
  usize cap;
  T *data;

  void init(Allocator alloc);
  void deinit();

  ~Vector() { deinit(); }

  bool is_empty() { return _len == 0; }
  usize len() { return _len; }

  void ensure_free_capacity(usize min_free_cap);
  void ensure_capacity(usize min_cap);
  void grow();

  void push(T x) { *push_empty() = x; }
  T *push_empty();

  T &operator[](usize idx) { return data[idx]; }

  T *end() { return data + _len; }

  Slice<T> slice() { return Slice<T>{data, _len}; }

  T shift_left() {
    T res = data[0];

    memmove(data, data + 1, (_len - 1) * sizeof(T));
    _len -= 1;

    return res;
  }

  Vector<T> move();
};

template<typename T> void Vector<T>::init(Allocator alloc) {
  this->alloc = alloc;

  _len = 0;
  cap  = 0;

  data = nullptr;
}

template<typename T> void Vector<T>::deinit() {
  if (data) {
    alloc.free(data, cap);
  }

  _len = 0;
  cap = 0;
  data = nullptr;
}

template<typename T> void Vector<T>::grow() {
  usize new_capacity = max<usize>(4, 2 * cap);
  ensure_capacity(new_capacity);
}

template<typename T> void Vector<T>::ensure_capacity(usize minimum_capacity) {
  usize new_capacity = max(cap, round_up_to_nearest_power_of_two(minimum_capacity));
  if (new_capacity == cap) {
    return;
  }
  data = alloc.realloc(data, cap, new_capacity);
  cap = minimum_capacity;
}

template<typename T> void Vector<T>::ensure_free_capacity(usize minimum_free_capacity) {
  usize new_capacity = round_up_to_nearest_power_of_two(_len + minimum_free_capacity);
  ensure_capacity(new_capacity);
}

template<typename T> T *Vector<T>::push_empty() {
  if (_len == cap) {
    grow();
  }

  T *res  = data + _len;
  _len   += 1;
  return res;
}

template<typename T> Vector<T> Vector<T>::move() {
  Vector<T> res = *this;
  memset(this, 0, sizeof(Vector<T>));
  return res;
}
