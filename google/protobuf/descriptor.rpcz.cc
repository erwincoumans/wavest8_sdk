// Generated by the protocol buffer compiler.  DO NOT EDIT!

#include "google/protobuf/descriptor.rpcz.h"
#include "google/protobuf/descriptor.pb.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/once.h>
#include <rpcz/rpcz.hpp>
namespace {
}  // anonymouse namespace

namespace google {
namespace protobuf {

void rpcz_protobuf_AssignDesc_google_2fprotobuf_2fdescriptor_2eproto() {
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "google/protobuf/descriptor.proto");
  GOOGLE_CHECK(file != NULL);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &rpcz_protobuf_AssignDesc_google_2fprotobuf_2fdescriptor_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
}

}  // namespace

void rpcz_protobuf_ShutdownFile_google_2fprotobuf_2fdescriptor_2eproto() {
}

}  // namespace protobuf
}  // namespace google
