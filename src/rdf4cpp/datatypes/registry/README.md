# Datatype Registry

The datatype registry of rdf4cpp stores information about
the registered [Literal](../../Literal.hpp) datatypes. The registry's operations are split into two categories, namely
runtime and compiletime operations. The datatypes' information is  availaible at compiletime and is stored in the library's read-only segment.

At runtime, the information about datatypes is type erased and stored in the [DatatypeRegistry](./DatatypeRegistry.hpp).
The type erasure is necessary, as the types of [Literals](../../Literal.hpp) are usually not available at compiletime. Hence, the respective behaviour of each datatype needs to be available at runtime.

## User Guide

Every datatype has a base set of functions. These functions are defined by the concept [LiteralDatatype](../LiteralDatatype.hpp).
This concept provides the most basic set of functions that are required to interact with a value of a particular datatype.
The requirements are enumerated in the following concept declaration, which can also be found in [LiteralDatatype.hpp](../LiteralDatatype.hpp).
Note that some entries are intentionally omitted for simplicity.

```c++
template<typename LiteralDatatypeImpl>
concept LiteralDatatype = requires(LiteralDatatypeImpl, std::string_view sv, typename LiteralDatatypeImpl::cpp_type const &cpp_value) {
                              typename LiteralDatatypeImpl::cpp_type;
                              { LiteralDatatypeImpl::from_string(sv) } -> std::convertible_to<typename LiteralDatatypeImpl::cpp_type>;
                              { LiteralDatatypeImpl::to_canonical_string(cpp_value) } -> std::convertible_to<std::string>;
                          };
```

The [LiteralDatatype::cpp_type](../LiteralDatatype.hpp) defines the corresponding C++ type of each particular type. Examples:
- xsd:string ~> std::string
- xsd:int ~> int32_t

Additionally, functions for converting this value _from_ and _to_ string have to be provided, i.e. [LiteralDatatype::from_string](../LiteralDatatype.hpp) and
[LiteralDatatype::to_canonical_string](../LiteralDatatype.hpp).
This set of default functions and definitions is determined by a specialization of [capabilities::Default](LiteralDatatypeImpl.hpp)
for any given datatype.

### A basic datatype
The following example defines a simplified version of `xsd:int` using only the default capability.

```c++
namespace rdf4cpp::datatypes::registry {
    inline constexpr util::ConstexprString basic_int = "http://basic-url.com#int";
        
    template<>
    struct DatatypeMapping<basic_int> {
        using cpp_datatype = int; // define the c++ representation of the type
    };
    
    template<>
    inline capabilities::Default<basic_int>::cpp_type capabilities::Default<basic_int>::from_string(std::string_view s) {
        // parse s into an int
    }
    
    template<>
    inline std::string capabilities::Default<basic_int>::to_canonical_string(cpp_type const &value) noexcept {
        // serialize value into a string
    }
}

namespace rdf4cp::rdf::datatypes::basic {
    // note: using an extern template for registry::LiteralDatatypeImpl<registry::basic_int> 
    // is advisable to reduce compile times
    struct Int : registry::LiteralDatatypeImpl<registry::basic_int> {};
}
```

### Value Capabilities
In addition to the default capability ([capabilities::Default](./LiteralDatatypeImpl.hpp)), there are also other capabilities that enable
a datatype to be used in more complex scenarios, without having to manually extract it from a [Literal](../../Literal.hpp)
using [Literal::value](../../Literal.hpp). The functions and mappings that need to be specialized are found in the definition of each capability.

- [capabilites::Numeric](./LiteralDatatypeImpl.hpp) / [NumericLiteralDatatype](../LiteralDatatype.hpp): allows [Literals](../../Literal.hpp) to be used as numbers. For example, the datatypes that implement (?) this capability enable [Literals](../../Literal.hpp) containing values of these datatypes to be added using the addition operator (`operator+`).
- [capabilities::Logical](./LiteralDatatypeImpl.hpp) / [LogicalLiteralDatatype](../LiteralDatatype.hpp): associates (?) [Literals](../../Literal.hpp) with an effective boolean value (`Literal::ebv`) and allows their values to participate in boolean operations (e.g., `operator&&`).
- [capabilities::Comparable](./LiteralDatatypeImpl.hpp) / [ComparableLiteralDatatype](../LiteralDatatype.hpp): enables [Literals](../../Literal.hpp) to paricipate in comparions operatorions (`operator<=>`). For the comparisons, the semantics of the underlying value are used.

#### Logical capability example
```c++
namespace rdf4cpp::datatypes::registry {
    inline constexpr util::ConstexprString basic_int = "http://basic-url.com#int";
        
    template<>
    struct DatatypeMapping<basic_int> {
        using cpp_datatype = int; // define the c++ representation of the type
    };
    
    template<>
    inline capabilities::Default<basic_int>::cpp_type capabilities::Default<basic_int>::from_string(std::string_view s) {
        // parse s into an int
    }
    
    template<>
    inline std::string capabilities::Default<basic_int>::to_canonical_string(cpp_type const &value) noexcept {
        // serialize value into a string
    }
    
    template<>
    inline bool capabilities::Logical<basic_int>::effective_boolean_value(cpp_type const &value) noexcept {
        return value != 0;
    }
}

namespace rdf4cp::rdf::datatypes::basic {
    // Note: again using an extern template for registry::LiteralDatatypeImpl<registry::basic_int> 
    // is advisable to reduce compile times
    // Note: must explicitly specify capability in definition
    struct Int : registry::LiteralDatatypeImpl<registry::basic_int, 
                                               registry::capabilities::Logical> {};
}
```

### Fixed Datatype Ids
By default, datatypes do not have fixed identifiers/indices in the registry.
As a result, searching for a particular datatype requires `O(log(n))` time.
To avoid this overhead, datatypes can be assigned a fixed identifier. This identifier places them in a fixed location in the registry; hence, they can be found in `O(1)` time.
To assign a fixed identifier to a datatype, two steps are required:

1. add an entry to the [reserved_datatype_ids](./FixedIdMappings.hpp) map
2. add the [capabilities::FixedId](./LiteralDatatypeImpl.hpp) capability to your type

### Value inlining
By default, values of literals are stored in the [NodeStorage](../../storage/README.md). 
However, to avoid the overhead of accessing it values of datatypes can also be packed
into the 42-bit [LiteralID](../../storage/identifier/README.md) of a
node storage handle. In turn, the value can be directly extracted from the handle, without having to access the [NodeStorage](../../storage/README.md).
Value inlining requires the specific value to actually fit into the available 42 bits.
However, it does not require that every possible value of a type fits into these available bits.
To enable value inlining, the capability [capabilities::Inlineable](./LiteralDatatypeImpl.hpp) needs to be specialized.
Notes: Inlining requires the datatype to have a fixed id as well. Addionally, inlining capable literals
are assumed to never need any form of escaping to convert them to their  n-triples string representation.
I.e. the output of their corresponding `to_string` function will be used without escaping it.

### Type System / Type Hierarchy
Datatypes can be arranged in a hierarchy. There are two types of relationships
in these hierarchies. `Subtype` relationships and `Promotion` relationships.

A subtype relationship narrows the value space of a parent datatype down to the value space of a derived datatype.
For example, we can define a subtype relationship between `xsd:long` and `xsd:int`, where `xsd:int` is a subtype (by restriction) of `xsd:long` as it narrows the value space of `xsd:long`.

A promotion relationship is any relationship between two types A and B 
where A is convertible/promotable to B but there exists no subtyping relationship between them.
For example `xsd:float` is convertible to `xsd:double` even though `xsd:float`s value space is not strictly a subset of the value
space of `xsd:double`. Therefore, this relationship is not a subtyping relationship but rather a promotion relationship.

The following example that uses a subset of `xsd` datatypes helps to better visualize these hierarchies.
Vertical connections (lines) represent subtype relationships and
horizontal connections (arrows) represent promotion relationships.

[reference](https://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#built-in-datatypes)
```
                          xsd:decimal  ─> xsd:float  ─>  xsd:double
                             │
                     ┌─  xsd:integer ─┐
                     │       │        │
xsd:nonPositiveInteger    xsd:long    xsd:nonNegativeInteger
                             │
                           xsd:int 
```

To construct these hierarchies at compiletime, two specializations can be provided.
Only provide the specialization for the needed relationship, i.e.
`xsd:int` would not define a promotion mapping, as it is not directly promotable.

#### Impl numericity vs Stub numericity
Earlier, the capability [capabilities::Numeric](./LiteralDatatypeImpl.hpp) was mentioned.
A type implementing this capability is called `impl-numeric` as it implements all required numeric operations.
There is also another kind of numericity; there are datatype that are numeric but do not explicitly implement the numeric operations. 
Instead, these datatypes rely on a supertype to implement the numeric operations for them.
This is the case for the integer hierarchy in `xsd` as types derived from `xsd:integer` do
not implement numeric operations themselves and instead rely on `xsd:integer` to perform them.
These types are called `stub-numeric` and instead of implementing [capabilities::Numeric](./LiteralDatatypeImpl.hpp)
they implement [capabilities::StubNumeric](./LiteralDatatypeImpl.hpp) which forwards all operations to
the specified `impl` type.

#### Specializations for Promotions
Ignore the nested template for now.

```c++
template<>
struct DatatypePromotionMapping<your_type> {
    using promoted = your_promoted_type;
};

template<>
struct DatatypeSupertypeMapping<your_type> {
    using supertype = your_super_type;
};
```

Additionally, like before, the capability must be explicitly declared
on the datatype you are defining.

```c++
struct YourType : registry::LiteralDatatypeImpl<registry::your_type,
                                                registry::capabilites::Promotable,
                                                registry::capabilites::Subtype> {};
```

The point of this hierarchy is to be able to convert values from one type to another (e.g., convert an int into a float).
Conversions are possible in both directions of the hierarchy. Conversions from subtype to supertype and promotions
must always be successful, as the value space widens, whereas conversions from supertype to subtype and demotions
can fail as the value space narrows.

Indirect conversions (i.e., conversions to non-direct neighbours in the graph) are by default defined as
the function composition of all direct conversions that make them up. For example
the conversion `xsd:int` -> `xsd:float` can be though of as the conversion chain
`xsd:int` -> `xsd:long` -> `xsd:integer` -> `xsd:decimal` -> `xsd:float`.

#### Specializing conversions
As mentioned above, the default conversions are obtained by `static_cast` and function composition.
To implement custom conversion behaviour the functions of [capabilities::Promotable](./LiteralDatatypeImpl.hpp) and
[capabilities::Subtype](./LiteralDatatypeImpl.hpp) can be specialized.

```c++
template<>
template<>
inline capabilities::Promotable<your_type>::promoted_cpp_type<IX> capabilities::Promotable::promote<IX>(cpp_type const &value) noexcept {
    // do some complex conversion
}

template<>
template<>
inline std::expected<cpp_type, DynamicError> capabilities::Promotable<your_type>::demote<IX>(promoted_cpp_type<IX> const &value) noexcept {
    // do some complex conversion or fail
}


template<>
template<>
inline capabilities::Subtype<your_type>::super_cpp_type<IX> capabilites::Subtype::into_supertype<IX>(cpp_type const &value) noexcept {
    // do some complex conversion
}

template<>
template<>
inline std::expected<cpp_type, DynamicError> capabilities::Subtype<your_type>::from_supertype<IX>(super_cpp_type<IX> const &value) noexcept {
    // do some complex conversion or fail
}
```

Here, the template parameter `IX` determines the distance of the conversion being specialized.
By default, the functions can only be specialized for `IX == 0`, i.e. we are specializing only the direct conversion (e.g. `xsd:decimal` -> `xsd:float`).
By default, conversions for further distances will be derived by function composition. To specialize conversions to further distances (e.g. `xsd:decimal` -> `xsd:double`),  [DatatypePromotionSpecializationOverride](./DatatypeMapping.hpp) or [DatatypeSupertypeSpecializationOverride](./DatatypeMapping.hpp)
need to be specialized first.

For example:

```c++
template<>
struct DatatypePromotionSpecializationOverride<xsd_decimal> {
    static constexpr size_t max_specialization_ix = 1;
};
```

You can then manually specialize:
```c++
template<>
template<>
inline capabilites::Promotable<xsd_decimal>::promoted_cpp_type<0> capabilites::Promotable::promote<0>(cpp_type const &value) noexcept {
    // xsd:decimal -> xsd:float
}

template<>
template<>
inline std::expected<cpp_type, DynamicError> capabilities::Promotable<xsd_decimal>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    // xsd:float -> xsd:decimal
}

template<>
template<>
inline capabilites::Promotable<xsd_decimal>::promoted_cpp_type<1> capabilites::Promotable::promote<1>(cpp_type const &value) noexcept {
    // xsd:decimal -> xsd:double
}

template<>
template<>
inline std::expected<cpp_type, DynamicError> capabilities::Promotable<xsd_decimal>::demote<1>(promoted_cpp_type<1> const &value) noexcept {
    // xsd:double -> xsd:decimal
}
```

The same applies to supertype conversions.


## Developer Guide
To understand this part of the README you should first read the **User Guide** above.

### Fundamentals

#### Representation of the Hierarchy as Conversion Tables
To be able to find conversions between types quickly, a so-called "conversion table" is used. It represents
(part of) the hierarchy as a search-friendly table. There is one table for each type that contains
all outgoing conversions and their respective inverse conversions.

The table is structured as follows: at index `(s, p)`
of the table for a type is the conversion that converts the source type to the type
that is s levels above the source and p levels right to the source, in that order.

For example the table for the simplified `xsd:int` from above looks like this

| s \ p | 0                           | 1                         | 2                          |
|-------|-----------------------------|---------------------------|----------------------------|
| 0     | `xsd:int` <-> `xsd:int`     |                           |                            |
| 1     | `xsd:int` <-> `xsd:long`    |                           |                            |
| 2     | `xsd:int` <-> `xsd:integer` |                           |                            |
| 3     | `xsd:int` <-> `xsd:decimal` | `xsd:int` <-> `xsd:float` | `xsd:int` <-> `xsd:double` |

#### Type Hierarchy Ranks
If a datatype is located in a hierarchy it has two ranks:
- **Subtype Rank**: this rank is the number of supertypes a given type has.
    - `xsd:int` has a subtype rank of 3
    - `xsd:long` has a subtype rank of 2
    - `xsd:float` has a subtype rank of 0

- **Promotion Rank**: similar to the subtype rank this rank is the number of promotions a type has.
    - `xsd:decimal` has a promotion rank of 2
    - `xsd:float` has a promotion rank of 1
    - `xsd:integer` has a promotion rank of 0

Ranks are used to perform calculations on the hierarchy, as the difference
between ranks of types can be used to index conversion tables.

Example: `xsd:int` -> `xsd:double`
1. `subtype_rank(xsd:int) == 3 and subtype_rank(xsd:double) == 0: Therefore s := 3`
2. `promotion_rank(into_supertype(xsd:int, 3)) == promotion_rank(xsd:decimal) == 2 and promotion_rank(xsd:double) == 0: Therefore p := 2`
3. Use conversion at index `(s, p) = (3, 2)` of the conversion table for `xsd:int`

### Conversion Table Implementation
This section briefly describes where the code that calculates conversion tables can be found.

First, during compiletime, all conversion tables and all conversions they contain are fully calculated. This is
done in [DatatypeConversion.hpp](DatatypeConversion.hpp) by traversing all outgoing edges for
each type. The table is calculated in chunks of 1d slices of the table and then stitched together.
[make_conversion_layer_impl](DatatypeConversion.hpp) calculates one 1d slice out of the 2d conversion table.
This slice can consist of either only promotions or only supertype conversions.
These 1d slices are then stitched together to form a full conversion table
in [make_conversion_table](DatatypeConversion.hpp).
The resulting conversion table is a `type list` of `type lists` in the layout described above.
Each inner type in the type lists is a struct with two static functions that perform the conversion in
either direction.

Second, once during the beginning of runtime, the calculated compiletime table is type erased and stored in the registry.
This is done using [RuntimeConversionTable::from_concrete](DatatypeConversionTyping.hpp)
in [DatatypeRegistry::add<>](DatatypeRegistry.hpp). Instead of being a `type list` of `type lists`
[RuntimeConversionTables](DatatypeConversionTyping.hpp) are a flattened 2d-`std::vector<>`.
This runtime representation can be searched in mostly the same way as the compiletime version.

Third, also during runtime, when searching for a conversion the functions [DatatypeRegistry::get_*_conversion](DatatypeRegistry.hpp)
are used to find the required conversion with the help of the runtime conversion tables
stored in the registry. The search is not quite as easy as described before, as this was only the most common
case, there could potentially be hierarchies or conversions that require slightly more complex logic.

### Defining Custom Capabilities
Defining capabilities requires five things
1. A template struct that can be specialized for datatypes (see [existing capabilities](./LiteralDatatypeImpl.hpp))
2. Corresponding type-erased functions in the datatype entries of the [registry](./DatatypeRegistry.hpp)
3. Logic to transform the compiletime definitions to type-erased definitions for the registry (see [DatatypeRegistry::add<>](./DatatypeRegistry.hpp))
4. A concept for convenience (see [LiteralDatatype](../LiteralDatatype.hpp) for details)
5. Logic in [Literal](../../Literal.hpp) that makes use of the new functions in the registry