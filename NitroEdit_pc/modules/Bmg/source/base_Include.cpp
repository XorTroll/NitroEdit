#include <base_Include.hpp>
#include <ntr/util/util_String.hpp>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QTextStream>

namespace {

    std::string FormatEscape(const ntr::fmt::BMG::MessageEscape &esc) {
        std::stringstream strm;
        strm << "{";
        for(ntr::u32 i = 0; i < esc.esc_data.size(); i++) {
            strm << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<ntr::u32>(esc.esc_data.at(i));
            if((i + 1) < esc.esc_data.size()) {
                strm << "-";
            }
        }
        strm << "}";
        return strm.str();
    }

}

QString FormatHexByteArray(const std::vector<ntr::u8> &data) {
    QByteArray arr(reinterpret_cast<const char*>(data.data()), data.size());
    return arr.toHex(' ');
}

bool ParseHexByteArray(const QString &raw, std::vector<ntr::u8> &out_arr) {
    out_arr.clear();
    auto raw_no_spaces = raw;
    raw_no_spaces.remove(QChar(' '), Qt::CaseInsensitive);
    for(int i = 0; i < raw_no_spaces.length(); i += 2) {
        bool ok;
        const auto byte_str = raw_no_spaces.mid(i, 2);
        const auto byte = static_cast<ntr::u8>(byte_str.toUShort(&ok, 16) & 0xFF);
        if(!ok) {
            return false;
        }
        out_arr.push_back(byte);
    }

    return true;
}

QString FormatEncoding(const ntr::fmt::BMG::Encoding enc) {
    switch(enc) {
        case ntr::fmt::BMG::Encoding::CP1252: {
            return "CP-1252";
        }
        case ntr::fmt::BMG::Encoding::UTF16: {
            return "UTF-16";
        }
        case ntr::fmt::BMG::Encoding::UTF8: {
            return "UTF-8";
        }
        case ntr::fmt::BMG::Encoding::ShiftJIS: {
            return "Shift JIS";
        }
    }
    return "<unk>";
}

ntr::Result ParseEncoding(const QString &raw_enc, ntr::fmt::BMG::Encoding &out_enc) {
    const auto raw_enc_lower = raw_enc.toLower();

    if((raw_enc_lower == "cp-1252") || (raw_enc_lower == "cp1252")) {
        out_enc = ntr::fmt::BMG::Encoding::CP1252;
        NTR_R_SUCCEED();
    }
    if((raw_enc_lower == "utf16") || (raw_enc_lower == "utf-16")) {
        out_enc = ntr::fmt::BMG::Encoding::UTF16;
        NTR_R_SUCCEED();
    }
    if((raw_enc_lower == "utf8") || (raw_enc_lower == "utf-8")) {
        out_enc = ntr::fmt::BMG::Encoding::UTF8;
        NTR_R_SUCCEED();
    }
    if((raw_enc_lower == "shiftjis") || (raw_enc_lower == "shift jis")) {
        out_enc = ntr::fmt::BMG::Encoding::ShiftJIS;
        NTR_R_SUCCEED();
    }

    return ntr::ResultBMGInvalidUnsupportedEncoding;
}

ntr::Result ParseMessage(const QString &msg, ntr::fmt::BMG::Message &out_msg) {
    out_msg = {};
    ntr::fmt::BMG::MessageToken cur_token = {};
    std::string cur_escape_byte;

    #define _NEDIT_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN { \
        if(cur_token.text.length() > 0) { \
            cur_token.type = ntr::fmt::BMG::MessageTokenType::Text; \
            out_msg.msg.push_back(cur_token); \
        } \
    }

    #define _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE { \
        ntr::u32 byte; \
        if(ntr::util::ConvertStringToNumber(cur_escape_byte, byte, 16)) { \
            cur_token.escape.esc_data.push_back(byte & 0xFF); \
            cur_escape_byte = ""; \
        } \
        else { \
            return ResultEditBMGInvalidEscapeByte; \
        } \
    }

    #define _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_OPENER \
        if(cur_token.type == ntr::fmt::BMG::MessageTokenType::Escape) { \
            return ResultEditBMGUnexpectedEscapeOpen; \
        } \
        _NEDIT_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN \
        cur_token = { \
            .type = ntr::fmt::BMG::MessageTokenType::Escape \
        }; \
        found_opener = false;

    #define _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_CLOSER \
        if(cur_token.type != ntr::fmt::BMG::MessageTokenType::Escape) { \
            return ResultEditBMGUnexpectedEscapeClose; \
        } \
        if(!cur_escape_byte.empty()) { \
            _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE \
        } \
        out_msg.msg.push_back(cur_token); \
        cur_token = {}; \
        found_closer = false;

    auto found_opener = false;
    auto found_closer = false;
    for(const auto &ch: msg.toStdString()) {
        if(ch == '{') {
            if(found_closer) {
                _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_CLOSER
            }

            if(found_opener) {
                // "{{" is treated as a normal "{" here
                cur_token.text += '{';
            }
            else {
                found_opener = true;
            }
        }
        else if(ch == '}') {
            if(found_opener) {
                _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_OPENER
            }

            if(found_closer) {
                // "}}" is treated as a normal "}" here
                cur_token.text += '}';
            }
            else {
                found_closer = true;
            }
        }
        else if(ch == '-') {
            if(found_opener) {
                _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_OPENER
            }
            if(found_closer) {
                _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_CLOSER
            }

            if(cur_token.type != ntr::fmt::BMG::MessageTokenType::Escape) {
                return ResultEditBMGUnexpectedEscapeClose;
            }
            if(cur_escape_byte.empty()) {
                return ResultEditBMGInvalidEscapeByte;
            }

            _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE
        }
        else {
            if(found_opener) {
                _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_OPENER
            }
            if(found_closer) {
                _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_CLOSER
            }

            if(cur_token.type == ntr::fmt::BMG::MessageTokenType::Escape) {
                cur_escape_byte += ch;
            }
            else {
                cur_token.text += ch;
            }
        }
    }

    if(found_opener) {
        _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_OPENER
    }
    if(found_closer) {
        _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_CLOSER
    }

    if(cur_token.type == ntr::fmt::BMG::MessageTokenType::Escape) {
        return ResultEditBMGUnclosedEscape;
    }

    _NEDIT_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN

    NTR_R_SUCCEED();
}

ntr::Result FormatMessage(const ntr::fmt::BMG::Message &msg, QString &out_str) {
    out_str = QString();
    for(const auto &msg_token: msg.msg) {
        switch(msg_token.type) {
            case ntr::fmt::BMG::MessageTokenType::Escape: {
                out_str += FormatEscape(msg_token.escape);
                break;
            }
            case ntr::fmt::BMG::MessageTokenType::Text: {
                auto msg_str = QString::fromStdU16String(msg_token.text);
                msg_str.replace("{", "{{");
                msg_str.replace("}", "}}");
                out_str += msg_str;
                break;
            }
        }
    }

    NTR_R_SUCCEED();
}

namespace {

    QDomDocument GenerateBmgXml(std::shared_ptr<ntr::fmt::BMG> &bmg) {
        QDomDocument doc("xml");

        auto bmg_elem = doc.createElement("bmg");
        bmg_elem.setAttribute("encoding", FormatEncoding(bmg->header.encoding));
        doc.appendChild(bmg_elem);
        
        for(const auto &msg: bmg->messages) {
            auto msg_elem = doc.createElement("message");
            if(bmg->HasMessageIds()) {
                msg_elem.setAttribute("id", QString("0x%1").arg(msg.id, 8, 16, QLatin1Char('0')));
            }
            
            if(bmg->info.GetAttributesSize() > 0) {
                msg_elem.setAttribute("attributes", FormatHexByteArray(msg.attrs));
            }

            for(const auto &token: msg.msg) {
                switch(token.type) {
                    case ntr::fmt::BMG::MessageTokenType::Text: {
                        auto token_text_elem = doc.createTextNode(QString::fromStdU16String(token.text));
                        msg_elem.appendChild(token_text_elem);
                        break;
                    }
                    case ntr::fmt::BMG::MessageTokenType::Escape: {
                        auto token_escape_elem = doc.createElement("escape");
                        token_escape_elem.setAttribute("data", FormatHexByteArray(token.escape.esc_data));
                        msg_elem.appendChild(token_escape_elem);
                        break;
                    }
                }
            }

            bmg_elem.appendChild(msg_elem);
        }

        return doc;
    }

    ntr::Result ParseFromXml(const QDomDocument &doc, std::shared_ptr<ntr::fmt::BMG> &out_bmg) {
        const auto root = doc.documentElement();

        ntr::u32 file_id = 0;
        ntr::fmt::BMG::Encoding enc;
        std::vector<ntr::fmt::BMG::Message> msgs;

        if(root.tagName() != "bmg") {
            NTR_R_FAIL(ResultLoadBMGXmlInvalidRootTag);
        }

        if(!root.hasAttribute("encoding")) {
            NTR_R_FAIL(ntr::ResultBMGInvalidUnsupportedEncoding);
        }
        const auto encoding_raw = root.attribute("encoding");
        NTR_R_TRY(ParseEncoding(encoding_raw, enc));

        if(root.hasAttribute("id")) {
            const auto file_id_raw = root.attribute("encoding");
            if(!ParseStringInteger(file_id_raw, file_id)) {
                NTR_R_FAIL(ResultBMGInvalidFileId);
            }
        }

        std::optional<bool> has_message_ids;
        std::optional<size_t> attribute_size;

        for(int i = 0; i < root.childNodes().size(); i++) {
            auto root_child = root.childNodes().at(i);
            if(!root_child.isElement()) {
                NTR_R_FAIL(ResultLoadBMGXmlInvalidChildTag);
            }

            auto msg = root_child.toElement();
            if(msg.tagName() != "message") {
                NTR_R_FAIL(ResultLoadBMGXmlInvalidChildTag);
            }

            ntr::fmt::BMG::Message cur_msg;

            if(msg.hasAttribute("id")) {
                const auto raw_id = msg.attribute("id");
                if(!ParseStringInteger(raw_id, cur_msg.id)) {
                    NTR_R_FAIL(ResultBMGInvalidMessageId);
                }

                if(!has_message_ids.has_value()) {
                    has_message_ids.emplace(true);
                }

                if(!has_message_ids.value()) {
                    NTR_R_FAIL(ResultLoadBMGXmlMessageIdMismatch);
                }
            }
            else {
                if(!has_message_ids.has_value()) {
                    has_message_ids.emplace(false);
                }

                if(has_message_ids.value()) {
                    NTR_R_FAIL(ResultLoadBMGXmlMessageIdMismatch);
                }
            }

            if(msg.hasAttribute("attributes")) {
                const auto attrs_raw = msg.attribute("attributes");
                if(!ParseHexByteArray(attrs_raw, cur_msg.attrs)) {
                    NTR_R_FAIL(ResultBMGInvalidAttributes);
                }

                if(!attribute_size.has_value()) {
                    attribute_size.emplace(cur_msg.attrs.size());
                }

                if(attribute_size.value() == 0) {
                    NTR_R_FAIL(ResultLoadBMGXmlAttributesMismatch);
                }
            }
            else {
                if(!attribute_size.has_value()) {
                    attribute_size.emplace(0);
                }

                if(attribute_size.value() > 0) {
                    NTR_R_FAIL(ResultLoadBMGXmlAttributesMismatch);
                }
            }

            for(int j = 0; j < msg.childNodes().size(); j++) {
                const auto msg_child = msg.childNodes().at(j);
                if(msg_child.isText()) {
                    const auto token = ntr::fmt::BMG::MessageToken {
                        .type = ntr::fmt::BMG::MessageTokenType::Text,
                        .text = msg_child.toText().data().toStdU16String()
                    };
                    cur_msg.msg.push_back(token);
                }
                else if(msg_child.isElement()) {
                    const auto msg_token = msg_child.toElement();
                    if(msg_token.tagName() != "escape") {
                        NTR_R_FAIL(ResultLoadBMGXmlInvalidMessageToken);
                    }
                    if(!msg_token.hasAttribute("data")) {
                        NTR_R_FAIL(ResultLoadBMGXmlInvalidMessageToken);
                    }

                    auto token = ntr::fmt::BMG::MessageToken {
                        .type = ntr::fmt::BMG::MessageTokenType::Escape
                    };
                    const auto raw_escape = msg_token.attribute("data");
                    if(!ParseHexByteArray(raw_escape, token.escape.esc_data)) {
                        NTR_R_FAIL(ResultLoadBMGXmlInvalidMessageToken);
                    }
                    cur_msg.msg.push_back(token);
                }
                else {
                    NTR_R_FAIL(ResultLoadBMGXmlInvalidMessageToken);
                }
            }

            msgs.push_back(cur_msg);
        }

        // Default values if no messages
        if(!has_message_ids.has_value()) {
            has_message_ids.emplace(false);
        }
        if(!attribute_size.has_value()) {
            attribute_size.emplace(0);
        }

        NTR_R_TRY(out_bmg->CreateFrom(enc, has_message_ids.value(), attribute_size.value(), msgs, file_id));

        NTR_R_SUCCEED();
    }

}

ntr::Result SaveBmgXml(std::shared_ptr<ntr::fmt::BMG> &bmg, const QString &path) {
    const auto bmg_xml = GenerateBmgXml(bmg);

    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) {
        NTR_R_FAIL(ntr::ResultUnableToOpenStdioFile);
    }

    QTextStream out(&file);
    bmg_xml.save(out, 0);
    file.close();

    NTR_R_SUCCEED();
}

ntr::Result LoadBmgXml(const QString &path, std::shared_ptr<ntr::fmt::BMG> &out_bmg) {
    QDomDocument doc;

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        NTR_R_FAIL(ntr::ResultUnableToOpenStdioFile);
    }

    if(!doc.setContent(&file)) {
        file.close();
        NTR_R_FAIL(ResultLoadBMGMalformedXml);
    }

    NTR_R_TRY(ParseFromXml(doc, out_bmg));

    file.close();
    NTR_R_SUCCEED();
}
