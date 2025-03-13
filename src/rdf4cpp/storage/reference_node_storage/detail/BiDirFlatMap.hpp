#ifndef RDF4CPP_RDF_REFERENCENODESTORAGE_BIDIRFLATMAP_HPP
#define RDF4CPP_RDF_REFERENCENODESTORAGE_BIDIRFLATMAP_HPP

#include <dice/sparse-map/sparse_set.hpp>
#include <dice/template-library/type_traits.hpp>
#include <rdf4cpp/storage/reference_node_storage/detail/IndexFreeList.hpp>

namespace rdf4cpp::storage::reference_node_storage::detail {

/**
 * A bidirectional map from Id to Backend
 *
 * @tparam Id id type (e.g. NodeID)
 * @tparam Backend stored value type (e.g. IRIBackend)
 * @tparam View view of Backend (e.g. IRIBackendView)
 * @tparam Hash hasher for View
 * @tparam Equal equality for View and Backend
 * @tparam Allocator allocator
 * @tparam ForwardVector the container type for the forward mapping, this should be a vector-like type
 * @tparam BackwardUnorderedSet the container type for the backward mapping, this should be an unordered_map-like type
 */
template<typename Id, typename Backend, typename View,
         typename Hash = std::hash<View>,
         typename Equal = std::equal_to<>,
         typename Allocator = std::allocator<Backend>,
         template<typename, typename> typename ForwardVector = std::vector,
         template<typename, typename, typename, typename> typename BackwardUnorderedSet = ::dice::sparse_map::sparse_set>
struct BiDirFlatMap {
    using id_type = Id;
    using backend_type = Backend;
    using view_type = View;
    using size_type = uint64_t;
    using allocator_type = Allocator;
    using hasher = Hash;
    using key_equal = Equal;

private:
    using forward_value_type = std::optional<backend_type>;
    using forward_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<forward_value_type>;
    using forward_type = ForwardVector<forward_value_type, forward_allocator_type>;

    struct backward_key_type {
        size_t hash; //< hash of the Value being looked up
        id_type id;  //< id into forward_
    };

    struct backward_key_equal {
        using is_transparent = void;

        using forward_const_pointer = typename std::allocator_traits<allocator_type>::template rebind_traits<forward_type>::const_pointer;

        forward_const_pointer forward_ptr; //< pointer to forward_ to be able to check for equality between View and Value
        [[no_unique_address]] Equal eq; //< eq for backend and view

        [[nodiscard]] forward_value_type const &access(backward_key_type const &k) const noexcept {
            return (*forward_ptr)[to_index(k.id)];
        }

        [[nodiscard]] bool forward_value_eq(forward_value_type const &a, forward_value_type const &b) const noexcept {
            if (a.has_value() && b.has_value()) {
                return eq(*a, *b);
            }

            return a.has_value() == b.has_value();
        }

        bool operator()(backward_key_type const &a, backward_key_type const &b) const noexcept {
            return a.id == b.id || (a.hash == b.hash && forward_value_eq(access(a), access(b)));
        }

        bool operator()(backward_key_type const &a, view_type const &b) const noexcept {
            auto const &ref = access(a);
            if (!ref.has_value()) {
                return false;
            }

            return eq(*ref, b);
        }

        bool operator()(view_type const &a, backward_key_type const &b) const noexcept {
            auto const &ref = access(b);
            if (!ref.has_value()) {
                return false;
            }

            return eq(a, *ref);
        }
    };

    struct backward_hasher {
        using is_transparent = void;

        [[no_unique_address]] Hash hash; //< hasher for views

        size_t operator()(backward_key_type const &a) const noexcept {
            return a.hash;
        }

        size_t operator()(view_type const &a) const noexcept {
            return hash(a);
        }
    };

    using backward_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<backward_key_type>;
    using backward_type = BackwardUnorderedSet<backward_key_type, backward_hasher, backward_key_equal, backward_allocator_type>;

    using index_free_list_bitmap_type = uint64_t;
    using index_free_list_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<index_free_list_bitmap_type>;
    using index_free_list_type = IndexFreeList<index_free_list_bitmap_type, index_free_list_allocator>;

    forward_type forward_;   //< forward_[to_index(id)] stores Value for Id id
    backward_type backward_; //< backward_[view] stores Id into forward_
    index_free_list_type freelist_; //< freelist for forward_
    [[no_unique_address]] allocator_type alloc_;

    /**
     * Translates the given id into an index into forward_
     */
    [[nodiscard]] static constexpr size_type to_index(id_type const id) noexcept {
        return static_cast<size_type>(id) - 1;
    }

    /**
     * Translates the given index into forward_ into an id
     */
    [[nodiscard]] static constexpr id_type to_id(size_type const ix) noexcept {
        return static_cast<id_type>(ix + 1);
    }

    template<typename Self>
    [[nodiscard]] static dice::template_library::copy_const_t<std::remove_reference_t<Self>, backend_type> *lookup_value_impl(Self &&self, id_type const id) noexcept {
        if (id == id_type{}) [[unlikely]] {
            return nullptr;
        }

        auto const ix = to_index(id);
        if (ix >= self.forward_.size() || !self.forward_[ix].has_value()) {
            return nullptr;
        }

        return &*self.forward_[ix];
    }

    template<typename Self>
    [[nodiscard]] static dice::template_library::copy_const_t<std::remove_reference_t<Self>, backend_type> &lookup_value_unchecked_impl(Self &&self, id_type const id) noexcept {
        auto const ix = to_index(id);
        assert(ix < self.size());
        assert(self.forward_[ix].has_value());
        return *self.forward_[ix];
    }

    template<typename Self, typename F>
    static void for_each_impl(Self &&self, F &&func) {
        for (size_t ix = 0; ix < self.forward_.size(); ++ix) {
            auto &fw = self.forward_[ix];
            if (fw.has_value()) {
                std::invoke(func, to_id(ix), *fw);
            }
        }
    }

public:
    explicit BiDirFlatMap(hasher const &hash = hasher{},
                          key_equal const &equal = key_equal{},
                          allocator_type const &alloc = allocator_type{}) noexcept : forward_{alloc},
                                                                                     backward_{0, backward_hasher{hash}, backward_key_equal{&forward_, equal}, alloc},
                                                                                     freelist_{alloc},
                                                                                     alloc_{alloc} {
    }

    explicit BiDirFlatMap(allocator_type const &alloc) noexcept : BiDirFlatMap{hasher{}, key_equal{}, alloc} {
    }

    // deleted because type is self-referential
    BiDirFlatMap(BiDirFlatMap const &) = delete;
    BiDirFlatMap(BiDirFlatMap &&) = delete;
    BiDirFlatMap &operator=(BiDirFlatMap const &) = delete;
    BiDirFlatMap &operator=(BiDirFlatMap &&) = delete;

    ~BiDirFlatMap() noexcept = default;

    /**
     * Number of elements stored in this map
     */
    [[nodiscard]] size_type size() const noexcept {
        return backward_.size();
    }

    /**
     * Requests the removal of unused capacity.
     */
    void shrink_to_fit() {
        forward_.shrink_to_fit();
        backward_.rehash(0); // force rehash (zero is special value)
        freelist_.shrink_to_fit();
    }

    /**
     * Look up the value corresponding to the given id
     *
     * @param id id for value to look up
     * @return if a value was found: a pointer to that value, otherwise nullptr
     */
    [[nodiscard]] backend_type const *lookup_value(id_type const id) const noexcept {
        return lookup_value_impl(*this, id);
    }

    [[nodiscard]] backend_type *lookup_value(id_type const id) noexcept {
        return lookup_value_impl(*this, id);
    }

    /**
     * Look up the value corresponding to the given id
     * Access is not checked.
     *
     * @param id id for value to look up
     * @return reference to the value
     */
    [[nodiscard]] backend_type const &lookup_value_unchecked(id_type const id) const noexcept {
        return lookup_value_unchecked_impl(*this, id);
    }

    [[nodiscard]] backend_type &lookup_value_unchecked(id_type const id) noexcept {
        return lookup_value_unchecked_impl(*this, id);
    }

    /**
     * Invoke a function on all (id, value) pairs in this map
     * @param func function to be invoked for each (id, value) pair
     */
    template<typename F> requires (std::invocable<F, id_type, backend_type const &>)
    void for_each(F &&func) const {
        return for_each_impl(*this, std::forward<F>(func));
    }

    template<typename F> requires (std::invocable<F, id_type, backend_type &>)
    void for_each(F &&func) {
        return for_each_impl(*this, std::forward<F>(func));
    }

    /**
     * Look up the id corresponding the given (view to a) value
     *
     * @param view view of value of which to find the id
     * @return id of the value if it was found, otherwise id_type{} if no id was found
     */
    [[nodiscard]] id_type lookup_id(view_type const &view) const noexcept {
        auto it = backward_.find(view);
        if (it == backward_.end()) {
            return id_type{};
        }

        return it->id;
    }

    /**
     * Look up the id (or ids, in case backwards is a multiset) corresponding the given view
     *
     * @param view view of value of which to find the id
     * @return range of ids of the value if there were any, otherwise empty range if no id was found
     */
    [[nodiscard]] std::ranges::range auto lookup_id_range(view_type const &view) const noexcept {
        auto [beg, end] = backward_.equal_range(view);
        return std::ranges::subrange(beg, end)
            | std::views::transform([](backward_key_type const &k) noexcept {
                return k.id;
            });
    }



    /**
     * Reserve capacity such that min_id is the first id that
     * triggers an allocation if it is inserted.
     */
    void reserve_until(id_type const min_id) {
        auto const new_size = to_index(min_id);
        if (new_size <= forward_.size()) {
            return;
        }

        forward_.resize(new_size);
        backward_.reserve(new_size);
        freelist_.occupy_until(new_size);
    }

    /**
     * Insert a value at the first free id
     *
     * @precondition the value is not yet present in this map
     *
     * @param view view of the value to construct
     * @param additional_args additional args to construct a value from the view
     * @return id of newly constructed value
     */
    template<typename ...Args>
    [[nodiscard]] id_type insert_assume_not_present(view_type const &view, Args &&...additional_args) {
        assert(lookup_id(view) == Id{});

        auto const assigned_ix = freelist_.occupy_next_available();
        if (assigned_ix >= forward_.size()) {
            assert(assigned_ix == forward_.size());
            forward_.emplace_back();
        }

        forward_[assigned_ix] = std::make_obj_using_allocator<backend_type>(alloc_, view, std::forward<Args>(additional_args)...);

        auto const assigned_id = to_id(assigned_ix);
        auto const h = backward_.hash_function().hash(view);
        backward_.emplace(h, assigned_id);

        return assigned_id;
    }

    /**
     * Insert a value at the given id
     *
     * @precondition the value is not yet present in this map
     * @precondition there is no value present at the requested id
     * @precondition sufficient capacity was allocated using reserve
     *
     * @param view view of the backend to construct
     * @param requested_id id to place the value at
     * @param additional_args additional arguments to construct a value from the view
     * @return id of newly constructed value
     */
    template<typename ...Args>
    void insert_assume_not_present_at(view_type const &view, id_type const requested_id, Args &&...additional_args) {
        assert(lookup_id(view) == id_type{});

        auto const lookup_ix = to_index(requested_id);
        assert(lookup_ix < forward_.size());
        assert(!forward_[lookup_ix].has_value());

        forward_[lookup_ix] = std::make_obj_using_allocator<backend_type>(alloc_, view, std::forward<Args>(additional_args)...);

        auto const h = backward_.hash_function().hash(view);
        backward_.emplace(h, requested_id);
    }

    /**
     * Erase the value at the given id
     *
     * @precondition there is actually a value at the given id
     * @param id id of the value to be erased
     */
    void erase_assume_present(id_type const id) {
        assert(lookup_value(id) != nullptr);

        auto const ix = to_index(id);
        auto &value = forward_[ix];
        auto const view = static_cast<view_type>(*value);

        auto [beg, end] = backward_.equal_range(view);
        auto it = std::find_if(beg, end, [id](backward_key_type const &k) noexcept {
            return k.id == id;
        });
        assert(it != backward_.end());

        backward_.erase(it);
        value.reset();
        freelist_.vacate(ix);
    }

    void clear() noexcept {
        forward_.clear();
        backward_.clear();
        freelist_.clear();
    }
};

} // namespace rdf4cpp::storage::reference_node_storage::detail

#endif // RDF4CPP_RDF_REFERENCENODESTORAGE_BIDIRFLATMAP_HPP
