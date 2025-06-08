template<typename T> struct Vector {
  Allocator alloc;
  usize len;
  usize cap;
  T *data;

  void init(Allocator alloc);
  void deinit();

  void grow();

  void push(T x) { *push_empty() = x; }
  T *push_empty();

  T &operator[](usize idx) {
    return data[idx];
  }

  Slice<T> slice() {
    return Slice<T>{ data, len };
  }

  Vector<T> move();
};

template<typename T>
void Vector<T>::init(Allocator alloc) {
  this->alloc = alloc;

  len = 0;
  cap = 0;

  data = nullptr;
}

template<typename T>
void Vector<T>::deinit() {
  if (data == nullptr) {
    return;
  }

  alloc.free(data, cap);
}

template<typename T>
void Vector<T>::grow() {
  usize new_cap = max<usize>(4, 2*cap);

  data = alloc.realloc(data, cap, new_cap);
  cap = new_cap;
}

template<typename T>
T *Vector<T>::push_empty() {
  if (len == cap) {
    grow();
  }

  T *res = data + len;
  len += 1;
  return res;
}

template<typename T>
Vector<T> Vector<T>::move() {
  Vector<T> res = *this;
  memset(this, 0, sizeof(Vector<T>));
  return res;
}
