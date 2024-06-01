cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libprotobuf" "" ${FOLDER_CONTEXT} FALSE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
    "any.cc"
    "any.pb.cc"
    "any_lite.cc"

    "arena.cc"
    "arenastring.cc"
    "arenaz_sampler.cc"

    "descriptor.cc"
    "descriptor.pb.cc"
    "descriptor_database.cc"

    "duration.pb.cc"
    "dynamic_message.cc"

    "empty.pb.cc"

    "extension_set.cc"
    "extension_set_heavy.cc"

    "field_mask.pb.cc"

    "generated_enum_util.cc"
    "generated_message_tctable_lite.cc"
    "generated_message_util.cc"
    "generated_message_bases.cc"
    "generated_message_reflection.cc"
    "generated_message_tctable_full.cc"
    "generated_message_tctable_lite.cc"
    "generated_message_util.cc"

    "implicit_weak_message.cc"
    "inlined_string_field.cc"

    "map.cc"
    "map_field.cc"

    "message.cc"
    "message_lite.cc"

    "parse_context.cc"

    "reflection_ops.cc"

    "repeated_field.cc"
    "repeated_ptr_field.cc"

    "service.cc"

    "source_context.pb.cc"
    "struct.pb.cc"

    "text_format.cc"

    "timestamp.pb.cc"
    "type.pb.cc"

    "unknown_field_set.cc"

    "wire_format.cc"
    "wire_format_lite.cc"

    "wrappers.pb.cc"
)

add_sources( SOURCE_GROUP "Include"
    "any.h"
    "any.pb.h"

    "arena.h"
    "arena_impl.h"
    "arenastring.h"
    "arenaz_sampler.h"

    "descriptor.h"
    "descriptor.pb.h"
    "descriptor_database.h"

    "duration.pb.h"
    "dynamic_message.h"

    "empty.pb.h"

    "endian.h"
    "explicitly_constructed.h"

    "extension_set.h"
    "extension_set_inl.h"

    "field_mask.pb.h"

    "generated_enum_util.h"
    "generated_message_tctable_decl.h"
    "generated_message_tctable_impl.h"
    "generated_message_util.h"
    "generated_message_bases.h"
    "generated_message_reflection.h"
    "generated_message_tctable_decl.h"
    "generated_message_tctable_impl.h"
    "generated_message_util.h"

    "has_bits.h"
    "implicit_weak_message.h"
    "inlined_string_field.h"

    "map.h"
    "map_entry_lite.h"
    "map_field_lite.h"
    "map_field_inl.h"

    "map_type_handler.h"
    "message_lite.h"
    "metadata_lite.h"
    "parse_context.h"
    "port.h"

    "reflection.h"
    "reflection_internal.h"
    "reflection_ops.h"

    "repeated_field.h"
    "repeated_ptr_field.h"

    "service.h"
    "string_member_robber.h"

    "source_context.pb.h"
    "struct.pb.h"

    "text_format.h"

    "timestamp.pb.h"
    "type.pb.h"

    "unknown_field_set.h"

    "wire_format.h"
    "wire_format_lite.h"

    "wrappers.pb.h"
)

add_sources( SOURCE_GROUP "Compiler"
    "compiler/importer.cc"
    "compiler/parser.cc"
)

add_sources( SOURCE_GROUP "Compiler/Include"
    "compiler/importer.h"
    "compiler/parser.h"
)

add_sources( SOURCE_GROUP "IO"
    "io/coded_stream.cc"
    "io/gzip_stream.cc"
    "io/io_win32.cc"
    "io/printer.cc"
    "io/strtod.cc"
    "io/tokenizer.cc"
    "io/zero_copy_stream.cc"
    "io/zero_copy_stream_impl.cc"
    "io/zero_copy_stream_impl_lite.cc"
)

add_sources( SOURCE_GROUP "IO/Include"
    "io/coded_stream.h"
    "io/gzip_stream.h"
    "io/io_win32.h"
    "io/printer.cc"
    "io/strtod.h"
    "io/tokenizer.h"
    "io/zero_copy_stream.h"
    "io/zero_copy_stream_impl.h"
    "io/zero_copy_stream_impl_lite.h"
)

add_sources( SOURCE_GROUP "Stubs"
    "stubs/bytestream.cc"
    "stubs/common.cc"
    "stubs/int128.cc"
    "stubs/status.cc"
    "stubs/statusor.cc"
    "stubs/stringpiece.cc"
    "stubs/stringprintf.cc"
    "stubs/structurally_valid.cc"
    "stubs/strutil.cc"
    "stubs/substitute.cc"
    "stubs/time.cc"
)

add_sources( SOURCE_GROUP "Stubs/Include"
    "stubs/bytestream.h"
    "stubs/callback.h"
    "stubs/casts.h"
    "stubs/common.h"
    "stubs/hash.h"
    "stubs/int128.h"
    "stubs/logging.h"
    "stubs/macros.h"
    "stubs/map_util.h"
    "stubs/mutex.h"
    "stubs/once.h"
    "stubs/platform_macros.h"
    "stubs/port.h"
    "stubs/status.h"
    "stubs/statusor.h"
    "stubs/stl_util.h"
    "stubs/stringpiece.h"
    "stubs/stringprintf.h"
    "stubs/strutil.h"
    "stubs/substitute.h"
    "stubs/template_util.h"
    "stubs/time.h"
)

add_sources( SOURCE_GROUP "Util"
    "util/delimited_message_util.cc"
    "util/field_comparator.cc"
    "util/field_mask_util.cc"
    "util/json_util.cc"
    "util/message_differencer.cc"
    "util/type_resolver_util.cc"
)

add_sources( SOURCE_GROUP "Util/Include"
    "util/delimited_message_util.h"
    "util/field_comparator.h"
    "util/field_mask_util.h"
    "util/json_util.h"
    "util/message_differencer.h"
    "util/type_resolver.h"
    "util/type_resolver_util.h"
)

add_sources( SOURCE_GROUP "Util/Internal"
    "util/internal/datapiece.cc"
    "util/internal/default_value_objectwriter.cc"
    "util/internal/error_listener.cc"
    "util/internal/field_mask_utility.cc"
    "util/internal/json_escaping.cc"
    "util/internal/json_objectwriter.cc"
    "util/internal/json_stream_parser.cc"
    "util/internal/object_writer.cc"
    "util/internal/proto_writer.cc"
    "util/internal/protostream_objectwriter.cc"
    "util/internal/protostream_objectsource.cc"
    "util/internal/type_info.cc"
    "util/internal/utility.cc"
)

add_sources( SOURCE_GROUP "Utils/Internal/Include"
    "util/internal/datapiece.h"
    "util/internal/default_value_objectwriter.h"
    "util/internal/error_listener.h"
    "util/internal/field_mask_utility.h"
    "util/internal/json_escaping.h"
    "util/internal/json_objectwriter.h"
    "util/internal/json_stream_parser.h"
    "util/internal/object_source.h"
    "util/internal/object_writer.h"
    "util/internal/proto_writer.h"
    "util/internal/protostream_objectwriter.h"
    "util/internal/protostream_objectsource.h"
    "util/internal/type_info.h"
    "util/internal/utility.h"
)

end_sources()
thirdparty_suppress_warnings()
