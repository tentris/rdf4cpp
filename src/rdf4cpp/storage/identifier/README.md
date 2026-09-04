# Identifiers Used to Store Nodes

## `NodeBackendHandle` (192 bit)
rdf4cpp uses `NodeBackendHandle` – a 192 bit wide type – to uniquely identify all `Node`s.
It consists of a `NodeBackendID` (64 bit), an instance-pointer to a node storage (64 bit),
and a vtable-pointer for a node storage (64 bit).
To retrieve a node backend, the full backend handle is required.
However, within a single, given node storage, `NodeBackendID` is able to fully identify a node.

## `NodeBackendID` (64 bit)
The `NodeBackendID` is a bit-packed structure.
The following figure gives an overview of its memory layout.
```
LSB                                                          MSB
┣━━━━━━━━━━━━━━━━━━━━ NodeBackendID (64 bit) ━━━━━━━━━━━━━━━━━━━━┫
├────────────── NodeID (56 bit) ───────────────────────┤├─┤├┤├───┤
├───────── LiteralID (50 bit) ───────────────────┤├────┤^  ^  ^
                                                    ^   |  |  |
                                                    |   |  |  free tagging bits (5 bit)
                                  LiteralType (6 bit)   |  |        
                                                        |  | 
                                                        |  |   
                                                        |  inlining tagging bit (1 bit)
                                                        |   
                                                        RDFNodeType (2 bit)
```

`NodeID` is the most frequently used identifier and is thus aligned with the LSB.
In the following, the bit-fields of `NodeBackendID` will be explained starting from the
most significant bits because the parts in the less significant depend on the bits in the more significant bits.

## Free Tagging Bits (5 bit)
The _free tagging bits_ can be used by the internal storage to store 13 bit of information in the `NodeBackendID`.
The IRIs of fixed-id datatypes can not have any tagging bits set, since the `NodeBackendID`s will be determined
by their `LiteralType`.

## Inlinining tagging bit (1 bit)
The _inlinining tagging bit_ indicates whether the value of the node is stored directly (in-place) inside the NodeID
instead of the node storage.

## `RDFNodeType` (2 bit)
The `RDFNodeType` specifies if the `Node` is a `BlankNode`, `IRI`, `Literal` or `Variable`.

## `NodeID` (56 bit)
The `NodeID` identifies a `Node` given `NodeType` inside a node storage.
A `NodeID` of 0 is considered a null `NodeID` which must not identify any resource.

The `NodeID` identifies a `Node` given `NodeType` inside a node storage.
A `NodeID` of 0 is considered a null `NodeID` which must not identify any resource.

### `LiteralType` (6 bit)
The `LiteralType` is a shortcut for the type stored within the backend of the `Literal`.
To reduce computation costs, common types like `xsd:int` can be encoded directly into the NodeBackendHandle.
If the type is directly encoded, the two MSB of the `LiteralType` are used to determine the 
type category the type belongs to (`Default`, `Numeric`, `Duration` or `Timepoint`).
For all other types, `LiteralType` is set to `0`. In that case, the type information must be retrieved from backend if needed.

### `LiteralID` (50 bit)
The `LiteralID` identifies a literal in a node storage, given `NodeType == Literal`.

#### `LiteralID` with inlined language tag
For `rdf:langString` only the language tag can be inlined into the `LiteralID`.
This uses the uppermost bits of the `LiteralID`. The amount of bits used depends on the configuration (default 2).
Inlining uses only as many bits as it needs, the remaining bits of the `LiteralID` is used for storing the
truncated `LiteralID`
If you are operating on `LiteralID`s directly, you need to check if they are inlined and possibly un-inline them before
using (the inlining tagging-bit of the `NodeBackendID` will be set to indicate that the language tag is inlined).

# Important Notes
- `NodeBackendID`s depend on the creation order if `Node`s within a `NodeStorage`.
  - They are not guaranteed to be equal between two executions.
  - `NodeID`s are not guaranteed to be equal between two `NodeStorage`s.
- When persisting a `NodeStorage` it is advisable to only persist `NodeBackendID`s and 
  not the full `NodeBackendHandle` otherwise the pointers inside it will point to nothing.
