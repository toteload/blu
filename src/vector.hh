#pragma once

template<typename T> struct Vector {
  Allocator alloc;
  usize _len;
  usize cap;
  T *data;

  void init(Allocator alloc);
  void deinit();

  bool is_empty() { return _len == 0; }
  usize len() { return _len; }

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
  if (data == nullptr) {
    return;
  }

  alloc.free(data, cap);
}

template<typename T> void Vector<T>::grow() {
  usize new_cap = max<usize>(4, 2 * cap);

  data = alloc.realloc(data, cap, new_cap);
  cap  = new_cap;
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
