#include <memory>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <iterator>
#include <initializer_list>
template<typename T, bool Extendable = false, typename Allocator = std::allocator<T>>
class circular_buffer {
    public:
    using alloc_traits = std::allocator_traits<Allocator>;
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;

    private:

    Allocator alloc_;
    T* data_ = nullptr;
    size_t head_ = 0; 
    size_t tail_ = 0;
    size_t size_= 0;
    size_t capacity_ = 0;


    public:


    size_type max_size() const {return alloc_traits::max_size(alloc_);}
    size_t size() const  {return size_;}
    size_t capacity() const {return capacity_;}
    allocator_type get_allocator() const {return alloc_;}

    circular_buffer(size_type n, const value_type& value) : capacity_(n) {
        if (n == 0) { data_ = nullptr; return; }
        data_ = alloc_traits::allocate(alloc_, capacity_);
        head_ = 0; size_ = 0; tail_ = 0;
        for (size_type i = 0; i < n; ++i) push_back(value);
    }
    circular_buffer(std::initializer_list<value_type> il) : capacity_(il.size()) {
        if (capacity_ == 0) { data_ = nullptr; return; }
        data_ = alloc_traits::allocate(alloc_, capacity_);
        head_ = tail_ = size_ = 0;
        for (auto& x : il) push_back(x);
    }
    circular_buffer(circular_buffer&& other, const allocator_type& a)
    : alloc_(a),
        data_(other.data_),
        head_(other.head_),
        tail_(other.tail_),
        size_(other.size_),
        capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.head_ = other.tail_ = other.size_ = other.capacity_ = 0;
    }


    circular_buffer& operator=(std::initializer_list<value_type> il) {
        clear();
        if (data_) { alloc_traits::deallocate(alloc_, data_, capacity_); data_ = nullptr; }
        capacity_ = il.size();
        if (capacity_ == 0) { head_ = tail_ = size_ = 0; return *this; }
        data_ = alloc_traits::allocate(alloc_, capacity_);
        head_ = tail_ = size_ = 0;
        for (auto& x : il) push_back(x);
        return *this;
}
    circular_buffer(){
        data_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        head_ = tail_ = 0;
    }
    circular_buffer(const allocator_type& allocator){
        alloc_ = allocator;
        data_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        head_ = tail_ = 0;
    }
    circular_buffer(size_t cap){
        data_ = alloc_traits::allocate(alloc_, cap);
        capacity_ = cap;
    };
    circular_buffer(size_t cap,const Allocator& allocator){
        alloc_ = allocator;
        capacity_ = cap;
        data_ = alloc_traits::allocate(alloc_, capacity_);
    }
    circular_buffer(const circular_buffer& other) {
        alloc_ = other.alloc_;
        T* new_data = alloc_traits::allocate(alloc_, other.capacity_);
        for (size_t i = 0;i < other.size_;++i){
            size_t index = (other.head_ + i) % other.capacity_;
            alloc_traits::construct(alloc_, new_data + i, *(other.data_ + index));
        }
        data_ = new_data;
        this->capacity_ = other.capacity_;
        this->size_= other.size_;
        this->head_ = 0;
        this->tail_ = size_;
        if (tail_ == capacity_) tail_ = 0;
    }
    circular_buffer(circular_buffer&& other) noexcept {
        data_ = other.data_;
        head_ = other.head_;
        tail_ = other.tail_;
        capacity_ = other.capacity_;
        size_= other.size_;
        alloc_ = std::move(other.alloc_);
        other.data_ = nullptr;
        other.head_ = 0;
        other.tail_ = 0;
        other.capacity_ = 0;
        other.size_= 0;

    }
    
    circular_buffer& operator=(circular_buffer&& other) noexcept {
        if (this == &other) return *this;
        if (data_ != nullptr){ for (size_t i = 0;i < this->size_;++i){
                size_t index = (this->head_ + i) % this->capacity_;
                alloc_traits::destroy(alloc_, this->data_ + index);
            }
            alloc_traits::deallocate(alloc_, this->data_ , this->capacity_);
        }
        this->data_ = other.data_;
        this->size_= other.size_;
        this->capacity_ = other.capacity_;
        this->head_ = other.head_;
        this->tail_ = other.tail_;
        alloc_ = std::move(other.alloc_);
        other.data_ = nullptr;
        other.capacity_ = 0;
        other.size_= 0;
        other.head_ = 0;
        other.tail_ = 0;
        return *this;
    }
    private:
    void grow_to(size_type need) {  
        if (need <= capacity_) return;

        size_type new_cap = (capacity_ == 0 ? 1 : capacity_);
        while (new_cap < need) new_cap *= 2;

        T* new_data = alloc_traits::allocate(alloc_, new_cap);
        for (size_type i = 0; i < size_; ++i) {
            size_type old_idx = (head_ + i) % capacity_;
            alloc_traits::construct(alloc_, new_data + i, std::move(data_[old_idx]));
        }
        if (data_) alloc_traits::deallocate(alloc_, data_, capacity_);
        data_ = new_data;
        capacity_ = new_cap;
        head_ = 0;
        tail_ = size_;
}
public:
    void resize(size_type n) { resize(n, value_type{}); }

    void resize(size_type n, const value_type& value) {
    if (n < size_) {
        while (size_ > n) pop_back();
    } else if (n > size_) {
        if (n > capacity_) {
            grow_to(n);
        }
        while (size_ < n) push_back(value);
    }
}
    void push_back(const T& object){
        if(size_< capacity_){ 
            alloc_traits::construct(alloc_, data_ + tail_, object);
            tail_++;
            if (tail_ == capacity_) tail_ = 0;
            size_++;
        }
        else if (size_== capacity_ && Extendable == true){
            grow_to(size_ + 1);
            alloc_traits::construct(alloc_, data_ + tail_, object);
            tail_++;
            if (tail_ == capacity_) tail_ = 0;
            size_++;
        }
        else if (size_== capacity_ && Extendable == false){
            alloc_traits::destroy(alloc_, data_ + tail_);
            alloc_traits::construct(alloc_, data_ + tail_, object);
            tail_++;
            if(tail_ == capacity_) tail_ = 0;
            head_++;
            if (head_ == capacity_) head_ = 0;
        }
    }
    void push_front(const T& object){
        if (size_< capacity_){
            if(head_ == 0){
                head_ = capacity_ - 1;
            }else head_--;
            alloc_traits::construct(alloc_, data_+head_,object);
            size_++;
        }
        else if (size_== capacity_ && Extendable == true){
            grow_to(size_ + 1);
            if(head_ == 0) head_ = capacity_ - 1;
            else head_--;
            alloc_traits::construct(alloc_, data_ + head_, object);
            size_++;
            tail_ = (head_ + size_) % capacity_;
        }
        else if (size_== capacity_ && Extendable == false){
            if(head_ == 0) head_ = capacity_ - 1;
            else head_--;
            alloc_traits::destroy(alloc_, data_ + head_);
            alloc_traits::construct(alloc_, data_ + head_,object);
            if (tail_ == 0) tail_ = capacity_ - 1;
            else tail_--;
        } 
    }
    bool empty() const noexcept {
        if(size_== 0) return true;
        else return false;
    }   
    void clear() {
        for (size_t i = 0;i < size_;++i){
            size_t index = (head_ + i) % capacity_;
            alloc_traits::destroy(alloc_, data_ + index);
        }
        head_ = tail_ = 0;
        size_= 0;
    }
    void pop_front(){
        if (size_== 0) throw std::out_of_range("буфер пуст");
        T object = *(data_ + head_);
        alloc_traits::destroy(alloc_, data_ + head_);
        size_--;
        if (head_ == capacity_ - 1) head_ = 0;
        else head_++;
    }
    void pop_back(){
        if (size_== 0) throw std::out_of_range("буфер пуст");
        size_t last_index = (tail_ == 0 ? capacity_ - 1 : tail_ - 1);
        T object = *(data_ + last_index);
        alloc_traits::destroy(alloc_, data_ + last_index);
        size_--;
        if (size_== 0) head_ = tail_ = 0;
        else tail_ = last_index;
    }
    /*erase and insert lately will be ready */
    circular_buffer& operator=(const circular_buffer& other){
        if (this == &other) return *this;
        this->clear();
        if (data_ != nullptr)alloc_traits::deallocate(alloc_, data_, capacity_);
        data_ = nullptr;
        alloc_ = other.alloc_;
        data_ = alloc_traits::allocate(alloc_, other.capacity_);
        this->size_= other.size_;
        this->capacity_ = other.capacity_;
        for (size_t i = 0;i < this->size_;++i){
            size_t index = (i + other.head_) % other.capacity_;
            alloc_traits::construct(alloc_, data_ + i, *(other.data_ + index));
        }
        head_ = 0;
        tail_ = size_;
        if(tail_ == capacity_) tail_ = 0;
        return *this;
    }
    T& at(size_t num){
        if(num >= size_) throw std::out_of_range("index out of range");
        size_t index = (head_ + num) % capacity_;
        return *(data_ + index);
    }
    const T& at(size_t num) const {
        if(num >= size_) throw std::out_of_range("index out of range");
        size_t index = (head_ + num) % capacity_;
        return *(data_ + index);
    }
    T& operator[](size_t num){
        size_t index = (head_ + num) % capacity_;
        return *(data_ + index);
    }
    const T& operator[](size_t num) const {
        size_t index = (head_ + num) % capacity_;
        return *(data_ + index);
    }
    bool operator==(const circular_buffer& other) const {
        if (size_ != other.size_) return false;
        for (size_t i = 0; i < size_; ++i) {
            if ((*this)[i] != other[i]) return false;
        }
        return true;
    }
    bool operator!=(const circular_buffer& other) const {
        return !(*this == other);
    }
    ~circular_buffer(){
        if (!data_) return;
        for(size_t i = 0;i < size_;++i){
            size_t index = (i + head_) % capacity_;
            alloc_traits::destroy(alloc_, data_ + index);
        }
        alloc_traits::deallocate(alloc_, data_, capacity_);
    }
    class iterator{
        friend class const_iterator;
        public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        private:
        difference_type pos_ = 0;
        circular_buffer* owner_ = nullptr;
        public:
        iterator (size_t pose = 0, circular_buffer* visitor = nullptr){
            owner_ = visitor;
            pos_ = pose;
        }

        circular_buffer* owner() const { return owner_; }
        difference_type pos() const { return pos_; }
        iterator& operator+=(difference_type num){
            pos_ += num;
            return *this;
        }
        iterator& operator-=(difference_type num){
            pos_ -= num;
            return *this;
        }
        T& operator*() const {
            if (owner_ != nullptr && pos_ >= 0 && pos_ < owner_->size_){
                auto index = ((owner_->head_ + static_cast<size_t>(pos_)) % owner_->capacity_);
                return owner_->data_[index];
            }
            else throw std::out_of_range("out of range");
        }
        T* operator->() const {
            return &(**this);
        }
        
        iterator& operator++(){
            pos_++;
            return *this;
        }
        iterator& operator--(){
            pos_--;
            return *this;
        }
        iterator operator++(int){
            iterator copy = *this;
            pos_++;
            return copy;
        }
        iterator operator--(int){
            iterator copy = * this;
            pos_--;
            return copy;
        }
        iterator operator+(difference_type n) const {
            iterator copy = *this;
            copy.pos_ += n;
            return copy;
        }
        iterator operator-(difference_type n) const {
            iterator copy = *this;
            copy.pos_ -= n;
            return copy;
        }
        difference_type operator-(const iterator& sec) const    {
            return (this->pos_ - sec.pos_);
        }
        T& operator[](difference_type n) const {
            return *(*this + n);
        }
        bool operator==(const iterator& other) const {
            return (this->pos_ == other.pos_ && this->owner_ == other.owner_);
        }
        bool operator!=(const iterator& other) const {
            return (this->pos_ != other.pos_ || this->owner_ != other.owner_);
        }
        bool operator>(const iterator& other) const {
            return (this->pos_ > other.pos_);
        }
        bool operator<(const iterator& other) const {
            return (this->pos_ < other.pos_);
        }
        bool operator>=(const iterator& other) const {
            return (this->pos_ >= other.pos_);
        }
        bool operator<=(const iterator& other) const {
            return (this->pos_ <= other.pos_);
        }
    };
    class const_iterator{
        public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        private:
        difference_type pos_ = 0;
        const circular_buffer* owner_ = nullptr;
        public:
        difference_type pos() const { return pos_;}
        const_iterator(const iterator& it) {
            owner_ = it.owner();
            pos_ = it.pos();
            }
        const_iterator (size_t pose = 0,const circular_buffer* visitor = nullptr){
            owner_ = visitor;
            pos_ = pose;
        }
        const_iterator& operator+=(difference_type num){
            pos_ += num;
            return *this;
        }
        const_iterator& operator-=(difference_type num){
            pos_ -= num;
            return *this;
        }
        const T& operator*() const {
            if (owner_ != nullptr && pos_ >= 0 && pos_ < owner_->size_){
                auto index = ((owner_->head_ + static_cast<size_t>(pos_)) % owner_->capacity_);
                return owner_->data_[index];
            }
            else throw std::out_of_range("out of range");
        }
        const T* operator->() const {
            return &(**this);
        }
        
        const_iterator& operator++(){
            pos_++;
            return *this;
        }
        const_iterator& operator--(){
            pos_--;
            return *this;
        }
        const_iterator operator++(int){
            const_iterator copy = *this;
            pos_++;
            return copy;
        }
        const_iterator operator--(int){
            const_iterator copy = * this;
            pos_--;
            return copy;
        }
        const_iterator operator+(difference_type n) const {
            const_iterator copy = *this;
            copy.pos_ += n;
            return copy;
        }
        const_iterator operator-(difference_type n) const {
            const_iterator copy = *this;
            copy.pos_ -= n;
            return copy;
        }
        difference_type operator-(const const_iterator& sec) const    {
            return (this->pos_ - sec.pos_);
        }
        const T& operator[](difference_type n) const {
            return *(*this + n);
        }
        bool operator==(const const_iterator& other) const {
            return (this->pos_ == other.pos_ && this->owner_ == other.owner_);
        }
        bool operator!=(const const_iterator& other) const {
            return (this->pos_ != other.pos_ || this->owner_ != other.owner_);
        }
        bool operator>(const const_iterator& other) const {
            return (this->pos_ > other.pos_);
        }
        bool operator<(const const_iterator& other) const {
            return (this->pos_ < other.pos_);
        }
        bool operator>=(const const_iterator& other) const {
            return (this->pos_ >= other.pos_);
        }
        bool operator<=(const const_iterator& other) const {
            return (this->pos_ <= other.pos_);
        }
    };
    iterator begin(){
        return iterator(0, this);
    }
    iterator end(){
        return iterator(size_, this);
    }
    const_iterator begin() const {
        return const_iterator(0, this);
    }
    const_iterator end() const {
        return const_iterator(size_, this);
    }
    class reverse_iterator{
        public:
        using difference_type = std::ptrdiff_t;
        private:
        iterator base_;
        public:
        reverse_iterator(iterator base){
            base_ = base;
        }
        iterator base() const {return base_;}
        reverse_iterator& operator++(){
            --base_;
            return *this;
        }
        reverse_iterator& operator--(){
            ++base_;
            return *this;
        }
        reverse_iterator operator++(int){
            reverse_iterator copy = *this;
            --base_;
            return copy;
        }
        reverse_iterator operator--(int){
            reverse_iterator copy = *this;
            ++base_;
            return copy;
        }
        bool operator==(const reverse_iterator& other) const{
            return base_ == other.base_;
        }
        bool operator!=(const reverse_iterator& other) const{
            return base_ != other.base_;
        }
        T& operator*() const {
            iterator copy = base_;
            --copy;
            return *copy;
        }
        T* operator->() const {
            iterator copy = base_;
            --copy;
            return &(*copy);
        }
    };
    class const_reverse_iterator{
        public:
        using difference_type = std::ptrdiff_t;
        private:
        const_iterator base_;
        public:
        const_reverse_iterator(const_iterator base){
            base_ = base;
        }
        const_iterator base() const {return base_;}
        const_reverse_iterator& operator++(){
            --base_;
            return *this;
        }
        const_reverse_iterator& operator--(){
            ++base_;
            return *this;
        }
        bool operator==(const const_reverse_iterator& other) const{
            return base_ == other.base_;
        }
        const_reverse_iterator operator++(int){
            const_reverse_iterator copy = *this;
            --base_;
            return copy;
        }
        const_reverse_iterator operator--(int){
            const_reverse_iterator copy = *this;
            ++base_;
            return copy;
        }
        bool operator!=(const const_reverse_iterator& other) const{
            return base_ != other.base_;
        }
        const T& operator*() const {
            const_iterator copy = base_;
            --copy;
            return *copy;
        }
        const T* operator->() const {
            const_iterator copy = base_;
            --copy;
            return &(*copy);
        }
    };
    reverse_iterator rbegin(){
        return reverse_iterator(end());
    }
    reverse_iterator rend(){
        return reverse_iterator(begin());
    }
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }
    const_iterator cbegin() const {
        return this->begin();
    }
    const_iterator cend() const {
        return this->end();
    }
    const_reverse_iterator crbegin() const { return rbegin(); }
    const_reverse_iterator crend() const { return rend(); }
    iterator replace(const_iterator pos,const value_type& value){
        difference_type p = pos - cbegin();
        if (p < 0) throw std::out_of_range("bad pos");
        size_type up = static_cast<size_type>(p);
        if (size_ < capacity_ && p < size_){
            size_type idx = (head_ + up) % capacity_;
            alloc_traits::destroy(alloc_, data_ + idx);
            alloc_traits::construct(alloc_, data_ + idx,value);
        }
        else throw std::out_of_range("nothing to change");
    }
    iterator insert(const_iterator pos, const value_type& value) {
    difference_type p = pos - cbegin();
    if (p < 0 || static_cast<size_type>(p) > size_) {
        throw std::out_of_range("bad pos");
    }
    size_type up = static_cast<size_type>(p);

    grow_to(size_ + 1);

    if (up == size_) {
        alloc_traits::construct(alloc_, data_ + tail_, value);
        ++size_;
        tail_ = (head_ + size_) % capacity_;
        return iterator(up, this);
    }

    size_type new_slot = (head_ + size_) % capacity_;
    size_type last     = (head_ + (size_ - 1)) % capacity_;
    alloc_traits::construct(alloc_, data_ + new_slot, std::move(data_[last]));

    for (size_type j = size_ - 1; j > up; --j) {
        size_type to   = (head_ + j) % capacity_;
        size_type from = (head_ + (j - 1)) % capacity_;
        data_[to] = std::move(data_[from]);
    }

    size_type at = (head_ + up) % capacity_;
    alloc_traits::destroy(alloc_, data_ + at);
    alloc_traits::construct(alloc_, data_ + at, value);

    ++size_;
    tail_ = (head_ + size_) % capacity_;
    return iterator(up, this);
    }
    iterator insert(const_iterator pos, size_type n, const value_type& value) {
    difference_type p = pos - cbegin();
    if (p < 0 || static_cast<size_type>(p) > size_) {
        throw std::out_of_range("bad pos");
    }
    if (n == 0) {
        return iterator(static_cast<size_type>(p), this);
    }
    size_type up = static_cast<size_type>(p);

    grow_to(size_ + n);

    if (up == size_) {
        for (size_type k = 0; k < n; ++k) {
            alloc_traits::construct(alloc_, data_ + tail_, value);
            tail_ = (tail_ + 1) % capacity_;
            ++size_;
        }
        return iterator(up, this);
    }

    size_type old_size = size_;
    for (size_type j = old_size; j > up; --j) {
        size_type to_pos   = j + n - 1;
        size_type from_pos = j - 1;

        size_type to   = (head_ + to_pos) % capacity_;
        size_type from = (head_ + from_pos) % capacity_;

        if (to_pos >= old_size) {
            alloc_traits::construct(alloc_, data_ + to, std::move(data_[from]));
        } else {
            data_[to] = std::move(data_[from]);
        }
    }

    for (size_type k = 0; k < n; ++k) {
        size_type posk = up + k;
        size_type phys = (head_ + posk) % capacity_;
        if (posk < old_size) alloc_traits::destroy(alloc_, data_ + phys);
        alloc_traits::construct(alloc_, data_ + phys, value);
    }

    size_ += n;
    tail_ = (head_ + size_) % capacity_;
    return iterator(up, this);
    }
    
    iterator erase(const_iterator i){
        if (size_ == 0) throw std::out_of_range("buffer is empty");
        if (i == this->cend()) throw std::out_of_range("out of range");
        std::ptrdiff_t index = i - this->cbegin();
        for (std::ptrdiff_t x = index;x < size_ - 1;++x){
            std::ptrdiff_t to = (x + this->head_) % capacity_;
            std::ptrdiff_t from = (x + this->head_ + 1) % capacity_;
            alloc_traits::destroy(alloc_, this->data_ + to);
            alloc_traits::construct(alloc_, this->data_ + to, *(this->data_ + from));
        }
        size_t last = (head_ + size_ - 1) % capacity_;
        alloc_traits::destroy(alloc_, this->data_ + last);
        size_--;
        tail_ = (head_ + size_) % capacity_;
        iterator new_it(index, this);  
        return new_it;
    }
    iterator erase(const_iterator start, const_iterator last){
        if (size_ == 0) throw std::out_of_range("buffer is empty");
        std::ptrdiff_t first = start - cbegin();
        std::ptrdiff_t last_phys = last - cbegin();
        if (first > last_phys) throw std::out_of_range("-");
        if (last_phys > size_) throw std::out_of_range("out of range");
        if(first < 0 || last_phys < 0) throw std::out_of_range("out of range");
        size_t difference = last_phys - first;
        if(difference == 0) return iterator(first, this);
        for (size_t idx = first;idx < size_ - difference;++idx){
            size_t index = (head_ + idx) % capacity_;
            size_t to = (head_ + idx) % capacity_;
            size_t from = (head_ + idx + difference) % capacity_;
            alloc_traits::destroy(alloc_, data_ + index);
            alloc_traits::construct(alloc_, data_ + to, *(data_ + from));
        }
        for (size_t last = size_ - difference;last < size_;++last){
            size_t phys = (head_ + last) % capacity_;
            alloc_traits::destroy(alloc_, data_ + phys);
        }
        size_ -= difference;
        tail_ = (head_ + size_) % capacity_;
        iterator new_it(first, this);
        return new_it;
    }
    void assign(size_type n, const value_type& value) {
    clear();
    if (n > capacity_) {
        grow_to(n);
    }
    for (size_type i = 0; i < n; ++i) push_back(value);
    }

    template<class It, std::enable_if_t<!std::is_integral_v<It>, int> = 0>
    void assign(It first, It last) {
    clear();
    for (; first != last; ++first) push_back(*first);
    }
        iterator insert(iterator pos, const value_type& value) {
        return insert(const_iterator(pos), value);
    }
    iterator insert(iterator pos, size_type n, const value_type& value) {
        return insert(const_iterator(pos), n, value);
    }

template<class It, std::enable_if_t<!std::is_integral_v<It>, int> = 0>
iterator insert(iterator pos, It first, It last) {
    size_type up = static_cast<size_type>(const_iterator(pos) - cbegin());
    size_type n = 0;
    for (It it = first; it != last; ++it) ++n;
    if (n == 0) return iterator(up, this);

    // гарантируем место (только для Extendable)
    if constexpr (Extendable) grow_to(size_ + n);
    else {
        if (size_ + n > capacity_) throw std::out_of_range("no capacity");
    }

    // сдвиг хвоста вправо на n
    for (size_type j = size_; j > up; --j) {
        size_type to_pos = j + n - 1;
        size_type from_pos = j - 1;
        size_type to = (head_ + to_pos) % capacity_;
        size_type from = (head_ + from_pos) % capacity_;
        if (to_pos >= size_) alloc_traits::construct(alloc_, data_ + to, std::move(data_[from]));
        else data_[to] = std::move(data_[from]);
    }

    // вставка диапазона
    size_type k = 0;
    for (It it = first; it != last; ++it, ++k) {
        size_type phys = (head_ + (up + k)) % capacity_;
        if (up + k < size_) alloc_traits::destroy(alloc_, data_ + phys);
        alloc_traits::construct(alloc_, data_ + phys, *it);
    }

    size_ += n;
    tail_ = (head_ + size_) % capacity_;
    return iterator(up, this);
    }

    iterator insert(iterator pos, std::initializer_list<value_type> il) {
        return insert(pos, il.begin(), il.end());
    }
    void assign(std::initializer_list<value_type> il) {
    assign(il.begin(), il.end());
    }
    T& front(){
        return data_[head_];
    }
    T& back(){
        return data_[(head_ + size_ - 1) % capacity_];
    }
    const T& front() const{
        return data_[head_];
    }
    const T& back() const {
        return data_[(head_ + size_ - 1) % capacity_];
    }
};