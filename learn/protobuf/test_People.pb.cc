// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: test_People.proto

#include "test_People.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace test {
PROTOBUF_CONSTEXPR People_ProjectsEntry_DoNotUse::People_ProjectsEntry_DoNotUse(
    ::_pbi::ConstantInitialized){}
struct People_ProjectsEntry_DoNotUseDefaultTypeInternal {
  PROTOBUF_CONSTEXPR People_ProjectsEntry_DoNotUseDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~People_ProjectsEntry_DoNotUseDefaultTypeInternal() {}
  union {
    People_ProjectsEntry_DoNotUse _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 People_ProjectsEntry_DoNotUseDefaultTypeInternal _People_ProjectsEntry_DoNotUse_default_instance_;
PROTOBUF_CONSTEXPR People::People(
    ::_pbi::ConstantInitialized)
  : list_()
  , projects_(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{})
  , name_(&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{})
  , age_(0){}
struct PeopleDefaultTypeInternal {
  PROTOBUF_CONSTEXPR PeopleDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~PeopleDefaultTypeInternal() {}
  union {
    People _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 PeopleDefaultTypeInternal _People_default_instance_;
}  // namespace test
static ::_pb::Metadata file_level_metadata_test_5fPeople_2eproto[2];
static const ::_pb::EnumDescriptor* file_level_enum_descriptors_test_5fPeople_2eproto[1];
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_test_5fPeople_2eproto = nullptr;

const uint32_t TableStruct_test_5fPeople_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  PROTOBUF_FIELD_OFFSET(::test::People_ProjectsEntry_DoNotUse, _has_bits_),
  PROTOBUF_FIELD_OFFSET(::test::People_ProjectsEntry_DoNotUse, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::test::People_ProjectsEntry_DoNotUse, key_),
  PROTOBUF_FIELD_OFFSET(::test::People_ProjectsEntry_DoNotUse, value_),
  0,
  1,
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::test::People, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::test::People, name_),
  PROTOBUF_FIELD_OFFSET(::test::People, age_),
  PROTOBUF_FIELD_OFFSET(::test::People, list_),
  PROTOBUF_FIELD_OFFSET(::test::People, projects_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, 8, -1, sizeof(::test::People_ProjectsEntry_DoNotUse)},
  { 10, -1, -1, sizeof(::test::People)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::test::_People_ProjectsEntry_DoNotUse_default_instance_._instance,
  &::test::_People_default_instance_._instance,
};

const char descriptor_table_protodef_test_5fPeople_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\021test_People.proto\022\004test\"\305\001\n\006People\022\014\n\004"
  "name\030\001 \001(\t\022\013\n\003age\030\002 \001(\005\022\014\n\004list\030\003 \003(\t\022,\n"
  "\010projects\030\027 \003(\0132\032.test.People.ProjectsEn"
  "try\032/\n\rProjectsEntry\022\013\n\003key\030\001 \001(\t\022\r\n\005val"
  "ue\030\002 \001(\t:\0028\001\"\027\n\006Corpus\022\r\n\tUNIVERSAL\020\000J\004\010"
  "\004\020\005J\004\010\017\020\020J\004\010\t\020\014R\003fooR\003barb\006proto3"
  ;
static ::_pbi::once_flag descriptor_table_test_5fPeople_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_test_5fPeople_2eproto = {
    false, false, 233, descriptor_table_protodef_test_5fPeople_2eproto,
    "test_People.proto",
    &descriptor_table_test_5fPeople_2eproto_once, nullptr, 0, 2,
    schemas, file_default_instances, TableStruct_test_5fPeople_2eproto::offsets,
    file_level_metadata_test_5fPeople_2eproto, file_level_enum_descriptors_test_5fPeople_2eproto,
    file_level_service_descriptors_test_5fPeople_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_test_5fPeople_2eproto_getter() {
  return &descriptor_table_test_5fPeople_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_test_5fPeople_2eproto(&descriptor_table_test_5fPeople_2eproto);
namespace test {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* People_Corpus_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_test_5fPeople_2eproto);
  return file_level_enum_descriptors_test_5fPeople_2eproto[0];
}
bool People_Corpus_IsValid(int value) {
  switch (value) {
    case 0:
      return true;
    default:
      return false;
  }
}

#if (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))
constexpr People_Corpus People::UNIVERSAL;
constexpr People_Corpus People::Corpus_MIN;
constexpr People_Corpus People::Corpus_MAX;
constexpr int People::Corpus_ARRAYSIZE;
#endif  // (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))

// ===================================================================

People_ProjectsEntry_DoNotUse::People_ProjectsEntry_DoNotUse() {}
People_ProjectsEntry_DoNotUse::People_ProjectsEntry_DoNotUse(::PROTOBUF_NAMESPACE_ID::Arena* arena)
    : SuperType(arena) {}
void People_ProjectsEntry_DoNotUse::MergeFrom(const People_ProjectsEntry_DoNotUse& other) {
  MergeFromInternal(other);
}
::PROTOBUF_NAMESPACE_ID::Metadata People_ProjectsEntry_DoNotUse::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_test_5fPeople_2eproto_getter, &descriptor_table_test_5fPeople_2eproto_once,
      file_level_metadata_test_5fPeople_2eproto[0]);
}

// ===================================================================

class People::_Internal {
 public:
};

People::People(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned),
  list_(arena),
  projects_(arena) {
  SharedCtor();
  if (arena != nullptr && !is_message_owned) {
    arena->OwnCustomDestructor(this, &People::ArenaDtor);
  }
  // @@protoc_insertion_point(arena_constructor:test.People)
}
People::People(const People& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      list_(from.list_) {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  projects_.MergeFrom(from.projects_);
  name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_name().empty()) {
    name_.Set(from._internal_name(), 
      GetArenaForAllocation());
  }
  age_ = from.age_;
  // @@protoc_insertion_point(copy_constructor:test.People)
}

inline void People::SharedCtor() {
name_.InitDefault();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  name_.Set("", GetArenaForAllocation());
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
age_ = 0;
}

People::~People() {
  // @@protoc_insertion_point(destructor:test.People)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    ArenaDtor(this);
    return;
  }
  SharedDtor();
}

inline void People::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  projects_.Destruct();
  name_.Destroy();
}

void People::ArenaDtor(void* object) {
  People* _this = reinterpret_cast< People* >(object);
  _this->projects_.Destruct();
}
void People::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void People::Clear() {
// @@protoc_insertion_point(message_clear_start:test.People)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  list_.Clear();
  projects_.Clear();
  name_.ClearToEmpty();
  age_ = 0;
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* People::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // string name = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_name();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "test.People.name"));
        } else
          goto handle_unusual;
        continue;
      // int32 age = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          age_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // repeated string list = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 26)) {
          ptr -= 1;
          do {
            ptr += 1;
            auto str = _internal_add_list();
            ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
            CHK_(ptr);
            CHK_(::_pbi::VerifyUTF8(str, "test.People.list"));
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<26>(ptr));
        } else
          goto handle_unusual;
        continue;
      // map<string, string> projects = 23;
      case 23:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 186)) {
          ptr -= 2;
          do {
            ptr += 2;
            ptr = ctx->ParseMessage(&projects_, ptr);
            CHK_(ptr);
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<186>(ptr));
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* People::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:test.People)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // string name = 1;
  if (!this->_internal_name().empty()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_name().data(), static_cast<int>(this->_internal_name().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "test.People.name");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_name(), target);
  }

  // int32 age = 2;
  if (this->_internal_age() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(2, this->_internal_age(), target);
  }

  // repeated string list = 3;
  for (int i = 0, n = this->_internal_list_size(); i < n; i++) {
    const auto& s = this->_internal_list(i);
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      s.data(), static_cast<int>(s.length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "test.People.list");
    target = stream->WriteString(3, s, target);
  }

  // map<string, string> projects = 23;
  if (!this->_internal_projects().empty()) {
    using MapType = ::_pb::Map<std::string, std::string>;
    using WireHelper = People_ProjectsEntry_DoNotUse::Funcs;
    const auto& map_field = this->_internal_projects();
    auto check_utf8 = [](const MapType::value_type& entry) {
      (void)entry;
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
        entry.first.data(), static_cast<int>(entry.first.length()),
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
        "test.People.ProjectsEntry.key");
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
        entry.second.data(), static_cast<int>(entry.second.length()),
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
        "test.People.ProjectsEntry.value");
    };

    if (stream->IsSerializationDeterministic() && map_field.size() > 1) {
      for (const auto& entry : ::_pbi::MapSorterPtr<MapType>(map_field)) {
        target = WireHelper::InternalSerialize(23, entry.first, entry.second, target, stream);
        check_utf8(entry);
      }
    } else {
      for (const auto& entry : map_field) {
        target = WireHelper::InternalSerialize(23, entry.first, entry.second, target, stream);
        check_utf8(entry);
      }
    }
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:test.People)
  return target;
}

size_t People::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:test.People)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // repeated string list = 3;
  total_size += 1 *
      ::PROTOBUF_NAMESPACE_ID::internal::FromIntSize(list_.size());
  for (int i = 0, n = list_.size(); i < n; i++) {
    total_size += ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
      list_.Get(i));
  }

  // map<string, string> projects = 23;
  total_size += 2 *
      ::PROTOBUF_NAMESPACE_ID::internal::FromIntSize(this->_internal_projects_size());
  for (::PROTOBUF_NAMESPACE_ID::Map< std::string, std::string >::const_iterator
      it = this->_internal_projects().begin();
      it != this->_internal_projects().end(); ++it) {
    total_size += People_ProjectsEntry_DoNotUse::Funcs::ByteSizeLong(it->first, it->second);
  }

  // string name = 1;
  if (!this->_internal_name().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_name());
  }

  // int32 age = 2;
  if (this->_internal_age() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_age());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData People::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSizeCheck,
    People::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*People::GetClassData() const { return &_class_data_; }

void People::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message* to,
                      const ::PROTOBUF_NAMESPACE_ID::Message& from) {
  static_cast<People *>(to)->MergeFrom(
      static_cast<const People &>(from));
}


void People::MergeFrom(const People& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:test.People)
  GOOGLE_DCHECK_NE(&from, this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  list_.MergeFrom(from.list_);
  projects_.MergeFrom(from.projects_);
  if (!from._internal_name().empty()) {
    _internal_set_name(from._internal_name());
  }
  if (from._internal_age() != 0) {
    _internal_set_age(from._internal_age());
  }
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void People::CopyFrom(const People& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:test.People)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool People::IsInitialized() const {
  return true;
}

void People::InternalSwap(People* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  list_.InternalSwap(&other->list_);
  projects_.InternalSwap(&other->projects_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &name_, lhs_arena,
      &other->name_, rhs_arena
  );
  swap(age_, other->age_);
}

::PROTOBUF_NAMESPACE_ID::Metadata People::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_test_5fPeople_2eproto_getter, &descriptor_table_test_5fPeople_2eproto_once,
      file_level_metadata_test_5fPeople_2eproto[1]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace test
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::test::People_ProjectsEntry_DoNotUse*
Arena::CreateMaybeMessage< ::test::People_ProjectsEntry_DoNotUse >(Arena* arena) {
  return Arena::CreateMessageInternal< ::test::People_ProjectsEntry_DoNotUse >(arena);
}
template<> PROTOBUF_NOINLINE ::test::People*
Arena::CreateMaybeMessage< ::test::People >(Arena* arena) {
  return Arena::CreateMessageInternal< ::test::People >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
