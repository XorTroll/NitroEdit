
#pragma once
#include <mod/mod_Module.hpp>
#include <ntr/fmt/fmt_BMG.hpp>
#include <QString>

constexpr ntr::Result ResultEditBMGInvalidEscapeByte = 0xe001;
constexpr ntr::Result ResultEditBMGUnexpectedEscapeOpen = 0xe002;
constexpr ntr::Result ResultEditBMGUnexpectedEscapeClose = 0xe003;
constexpr ntr::Result ResultEditBMGUnclosedEscape = 0xe004;
constexpr ntr::Result ResultBMGInvalidMessageId = 0xe005;
constexpr ntr::Result ResultBMGInvalidAttributes = 0xe006;
constexpr ntr::Result ResultLoadBMGMalformedXml = 0xe007;
constexpr ntr::Result ResultLoadBMGXmlInvalidRootTag = 0xe008;
constexpr ntr::Result ResultLoadBMGXmlInvalidChildTag = 0xe009;
constexpr ntr::Result ResultLoadBMGXmlMessageIdMismatch = 0xe00a;
constexpr ntr::Result ResultLoadBMGXmlAttributesMismatch = 0xe00b;
constexpr ntr::Result ResultLoadBMGXmlInvalidMessageToken = 0xe00c;

constexpr ntr::ResultDescriptionEntry ResultDescriptionTable[] = {
    { ResultEditBMGInvalidEscapeByte, "Invalid escape byte found in BMG text" },
    { ResultEditBMGUnexpectedEscapeOpen, "Unexpected escape opening found in BMG text" },
    { ResultEditBMGUnexpectedEscapeClose, "Unexpected escape closing found in BMG text" },
    { ResultEditBMGUnclosedEscape, "Reached BMG text and with unclosed escape" },
    { ResultBMGInvalidMessageId, "Invalid BMG message ID integer" },
    { ResultBMGInvalidAttributes, "Invalid BMG attributes hex byte array" },
    { ResultLoadBMGMalformedXml, "Malformed XML file format" },
    { ResultLoadBMGXmlInvalidRootTag, "Invalid XML file to parse as BMG: expected root 'bmg' element" },
    { ResultLoadBMGXmlInvalidChildTag, "Invalid XML file to parse as BMG: expected child 'message' element" },
    { ResultLoadBMGXmlMessageIdMismatch, "Invalid XML file to parse as BMG: some messages have ID and others do not" },
    { ResultLoadBMGXmlAttributesMismatch, "Invalid XML file to parse as BMG: messages have different attributes size" },
    { ResultLoadBMGXmlInvalidMessageToken, "Invalid XML file to parse as BMG: invalid message token (expected plain text or escape token)" }
};

inline constexpr ntr::Result GetResultDescription(const ntr::Result rc, std::string &out_desc) {
    NTR_R_TRY(ntr::GetResultDescription(rc, out_desc));

    for(ntr::u32 i = 0; i < std::size(ResultDescriptionTable); i++) {
        if(ResultDescriptionTable[i].first.value == rc.value) {
            out_desc = ResultDescriptionTable[i].second;
            NTR_R_SUCCEED(); 
        }
    }

    NTR_R_FAIL(ntr::ResultUnknownResult);
}

inline std::string FormatResult(const ntr::Result rc) {
    std::string desc = "<unknown error: " + QString::number(rc.value, 16).toStdString() + ">";
    ::GetResultDescription(rc, desc);
    return desc;
}

QString FormatHexByteArray(const std::vector<ntr::u8> &data);
bool ParseHexByteArray(const QString &raw, std::vector<ntr::u8> &out_arr);

template<typename T>
inline bool ParseStringInteger(const QString &raw, T &out_val) {
    // Try decimal first, hex otherwise
    bool parse_ok = false;

    const auto dec_val = raw.toInt(&parse_ok);
    if(parse_ok) {
        out_val = static_cast<T>(dec_val);
        return true;
    }

    const auto hex_val = raw.toInt(&parse_ok, 16);
    if(parse_ok) {
        out_val = static_cast<T>(hex_val);
        return true;
    }

    return false;
}

#define NEDIT_MOD_BMG_FORMAT_BMG_EXTENSION "bmg"
#define NEDIT_MOD_BMG_FORMAT_BMG_FILTER "Binary message file (*." NEDIT_MOD_BMG_FORMAT_BMG_EXTENSION ")"

#define NEDIT_MOD_BMG_FORMAT_XML_EXTENSION "xml"
#define NEDIT_MOD_BMG_FORMAT_XML_FILTER "XML message file (*." NEDIT_MOD_BMG_FORMAT_XML_EXTENSION ")"

QString FormatEncoding(const ntr::fmt::BMG::Encoding enc);
ntr::Result ParseEncoding(const QString &raw_enc, ntr::fmt::BMG::Encoding &out_enc);

ntr::Result ParseMessage(const QString &msg, ntr::fmt::BMG::Message &out_msg);
ntr::Result FormatMessage(const ntr::fmt::BMG::Message &msg, QString &out_str);

ntr::Result SaveBmgXml(std::shared_ptr<ntr::fmt::BMG> &bmg, const QString &path);
ntr::Result LoadBmgXml(const QString &path, std::shared_ptr<ntr::fmt::BMG> &out_bmg);
