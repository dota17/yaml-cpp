#include <cassert>
#include <sstream>

#include "yaml-cpp/emitfromevents.h"
#include "yaml-cpp/emitter.h"
#include "yaml-cpp/emittermanip.h"
#include "yaml-cpp/null.h"
#include "yaml-cpp/node/convert.h"

namespace YAML {
struct Mark;
}  // namespace YAML

namespace {
std::string ToString(YAML::anchor_t anchor) {
  std::stringstream stream;
  stream << anchor;
  return stream.str();
}

template <typename T>
bool IsNumber(const std::string& value) {
  T rhs;
  std::stringstream stream(value);
  if (value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0) {
    stream >> std::hex;
  } else if (value.rfind("0", 0) == 0) {
    stream >> std::oct;
  } else {
    stream >> std::dec;
  }
  if ((stream >> std::noskipws >> rhs) && (stream >> std::ws).eof()) {
    return true;
  }
  return false;
}

bool IsBool(const std::string& value) {
  return value == "true" || value == "True" || value == "TRUE" ||
      value == "false" || value == "False" || value == "FALSE";
}

bool IsWriteQuotation(const std::string& tag, const std::string& value) {
  return (tag == "!") ? (IsNumber<int>(value) || IsNumber<double>(value) ||
                         IsBool(value) || YAML::conversion::IsNaN(value) ||
                         YAML::conversion::IsInfinity(value))
                      : false;
}
}  // namespace

namespace YAML {
EmitFromEvents::EmitFromEvents(Emitter& emitter)
    : m_emitter(emitter), m_stateStack{} {}

void EmitFromEvents::OnDocumentStart(const Mark&) {}

void EmitFromEvents::OnDocumentEnd() {}

void EmitFromEvents::OnNull(const Mark&, anchor_t anchor) {
  BeginNode();
  EmitProps("", anchor);
  m_emitter << Null;
}

void EmitFromEvents::OnAlias(const Mark&, anchor_t anchor) {
  BeginNode();
  m_emitter << Alias(ToString(anchor));
}

void EmitFromEvents::OnScalar(const Mark&, const std::string& tag,
                              anchor_t anchor, const std::string& value) {
  BeginNode();
  EmitProps(tag, anchor);
  if (IsWriteQuotation(tag, value)) {
    m_emitter << DoubleQuoted;
  }
  m_emitter << value;
}

void EmitFromEvents::OnSequenceStart(const Mark&, const std::string& tag,
                                     anchor_t anchor,
                                     EmitterStyle::value style) {
  BeginNode();
  EmitProps(tag, anchor);
  switch (style) {
    case EmitterStyle::Block:
      m_emitter << Block;
      break;
    case EmitterStyle::Flow:
      m_emitter << Flow;
      break;
    default:
      break;
  }
  m_emitter << BeginSeq;
  m_stateStack.push(State::WaitingForSequenceEntry);
}

void EmitFromEvents::OnSequenceEnd() {
  m_emitter << EndSeq;
  assert(m_stateStack.top() == State::WaitingForSequenceEntry);
  m_stateStack.pop();
}

void EmitFromEvents::OnMapStart(const Mark&, const std::string& tag,
                                anchor_t anchor, EmitterStyle::value style) {
  BeginNode();
  EmitProps(tag, anchor);
  switch (style) {
    case EmitterStyle::Block:
      m_emitter << Block;
      break;
    case EmitterStyle::Flow:
      m_emitter << Flow;
      break;
    default:
      break;
  }
  m_emitter << BeginMap;
  m_stateStack.push(State::WaitingForKey);
}

void EmitFromEvents::OnMapEnd() {
  m_emitter << EndMap;
  assert(m_stateStack.top() == State::WaitingForKey);
  m_stateStack.pop();
}

void EmitFromEvents::BeginNode() {
  if (m_stateStack.empty())
    return;

  switch (m_stateStack.top()) {
    case State::WaitingForKey:
      m_emitter << Key;
      m_stateStack.top() = State::WaitingForValue;
      break;
    case State::WaitingForValue:
      m_emitter << Value;
      m_stateStack.top() = State::WaitingForKey;
      break;
    default:
      break;
  }
}

void EmitFromEvents::EmitProps(const std::string& tag, anchor_t anchor) {
  if (!tag.empty() && tag != "?" && tag != "!")
    m_emitter << VerbatimTag(tag);
  if (anchor)
    m_emitter << Anchor(ToString(anchor));
}
}  // namespace YAML
