#pragma once
#include <string>

// 単純なパス処理のユーティリティクラス
class PathUtils {
public:
    // ファイルパスからディレクトリ部分を取得
    static std::string GetDirectoryPath(const std::string& filePath) {
        const char* pathSeparators = "/\\";
        size_t lastSeparatorPos = filePath.find_last_of(pathSeparators);

        if (lastSeparatorPos != std::string::npos) {
            return filePath.substr(0, lastSeparatorPos);
        }

        return "";
    }

    // パスを結合
    static std::string CombinePath(const std::string& path1, const std::string& path2) {
        if (path1.empty()) {
            return path2;
        }

        char lastChar = path1[path1.size() - 1];
        if (lastChar == '/' || lastChar == '\\') {
            return path1 + path2;
        }

        return path1 + '/' + path2;
    }

    // ファイル名の拡張子を取得
    static std::string GetFileExtension(const std::string& filePath) {
        size_t dotPos = filePath.find_last_of('.');

        if (dotPos != std::string::npos) {
            return filePath.substr(dotPos + 1);
        }

        return "";
    }

    // ファイルパスからファイル名を取得
    static std::string GetFileName(const std::string& filePath) {
        const char* pathSeparators = "/\\";
        size_t lastSeparatorPos = filePath.find_last_of(pathSeparators);

        if (lastSeparatorPos != std::string::npos) {
            return filePath.substr(lastSeparatorPos + 1);
        }

        return filePath;
    }
};