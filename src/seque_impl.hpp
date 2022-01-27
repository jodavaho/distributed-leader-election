
template <typename T, std::size_t N>
StaticQueue<T,N>::StaticQueue():
  idx_front(0), idx_back(0), count(0)
{
}

template <typename T, std::size_t N>
StaticQueue<T,N>::~StaticQueue()
{
}

template <typename T, std::size_t N>
bool StaticQueue<T,N>::is_full() const{
  return size()==N;
}

template <typename T, std::size_t N>
bool StaticQueue<T,N>::is_empty() const{
  return size()==0;;
}

template <typename T, std::size_t N>
size_t StaticQueue<T,N>::size() const{
  return count;
}

template <typename T, std::size_t N>
QueueRetcode StaticQueue<T,N>::push(const T item){
  if (is_full())
  {
    return ERR_QUEUE_FULL;
  }

  circle_buf[idx_back]=item;

  count ++;

  //ideally, idx_back=(idx_back+1)%N
  //but we can't assume N = 2^n in general
  idx_back ++;
  if (idx_back >= N)
  {
    idx_back = 0;
  } 

  return OK;
}

template <typename T, std::size_t N>
QueueRetcode StaticQueue<T,N>::front(T &out_item) const {
  if (is_empty())
  {
    return ERR_QUEUE_EMPTY;
  }

  out_item = circle_buf[idx_front];
  return OK;

}

template <typename T, std::size_t N>
QueueRetcode StaticQueue<T,N>::pop(){
  if (is_empty())
  {
    return ERR_QUEUE_EMPTY;
  }

  count --;

  //ideally, idx_back=(idx_back+1)%N
  //but we can't assume N = 2^n in general
  idx_front ++;
  if (idx_front >= N)
  {
    idx_front = 0;
  }
  return OK;
}

template <typename T, std::size_t N>
QueueRetcode StaticQueue<T,N>::pop(T &out_item){
  auto r = front(out_item);
  if (!Q_OK(r)){
    return r;
  }
  r = pop();
  return r;
}
