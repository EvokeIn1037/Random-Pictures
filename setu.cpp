#include "setu.h"
#include "./ui_setu.h"

#include <windows.h>
#include <QPixmap>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

double LABELRATIO = double(700) / double(370);
std::string URL = "https://api.lolicon.app/setu/v2";
std::wstring TITLE = L"";
std::wstring AUTHOR = L"";
std::wstring UID = L"";
std::wstring TAGS = L"";
std::string PICPATH = "";
using json = nlohmann::json;  // Define json as shorthand for nlohmann::json
bool HASPIC = false;

Setu::Setu(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Setu)
{
    ui->setupUi(this);

    // Connect the QPushButton's clicked signal to the custom slot
    connect(ui->pushButton, &QPushButton::clicked, this, &Setu::handleButtonClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &Setu::handleClearButtonClicked);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &Setu::Setu::orgPicClicked);
}

Setu::~Setu()
{
    delete ui;
}

std::string urlEncode(const std::string& input) {
    std::ostringstream escaped;
    escaped.fill('0');

    for (unsigned char c : input) {
        if (c == ' ') {
            escaped << '%';
            escaped << '2';
            escaped << '0';
        } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::uppercase << std::setw(2) << std::hex << static_cast<int>(c);
        }
    }

    return escaped.str();
}

int getRequestURL(QString inTags, QString inUid, bool checked, bool checked2, bool checked3, bool checked4) {
    std::string intags = inTags.toUtf8().constData();
    std::string inuid = inUid.toUtf8().constData();

    int len = intags.length(), ulen = inuid.length();

    if (intags[0] == '/' || intags[0] == '+' || intags[len - 1] == '/' || intags[len - 1] == '+') return 1;
    int substart = 0, sublen = 1, orcnt = 0, andcnt = 0;
    if(len > 0) URL += "?tag=";
    for (int i = 1; i < len; i++) {
        if (intags[i] == '/') {
            if (++orcnt > 20) return 1;
            if (intags[i - 1] == '/' || intags[i - 1] == '+') return 1;
            URL += (urlEncode(intags.substr(substart, sublen)) + '|');
            substart = i + 1;
            sublen = 0;
        }
        else if (intags[i] == '+') {
            if (++andcnt > 3) return 1;
            if (intags[i - 1] == '/' || intags[i - 1] == '+') return 1;
            URL += (urlEncode(intags.substr(substart, sublen)) + "&tag=");
            substart = i + 1;
            sublen = 0;
            orcnt = 0;
        }
        else sublen++;
    }
    URL += urlEncode(intags.substr(substart, sublen));

    if (ulen > 0) {
        std::string::const_iterator it = inuid.begin();
        while (it != inuid.end()) {
            if (!std::isdigit(*it)) return 2;
            ++it;
        }
        if (len == 0) URL += ("?uid=" + inuid);
        else URL += ("&uid=" + inuid);
    }

    if (checked) {
        if (len == 0 && ulen == 0) URL += "?r18=1";
        else URL += "&r18=1";
    }

    if (checked2 && checked3) return 3;
    if (checked2) {
        if (len == 0 && ulen == 0) URL += "?aspectRatio=gt1";
        else URL += "&aspectRatio=gt1";
    }
    else if (checked3) {
        if (len == 0 && ulen == 0) URL += "?aspectRatio=lt1";
        else URL += "&aspectRatio=lt1";
    }

    if (checked4) {
        if (len == 0 && ulen == 0) URL += "?excludeAI=true";
        else URL += "&excludeAI=true";
    }

    return 0;
}

// Callback function to handle the data received from the server
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* outString) {
    size_t totalSize = size * nmemb;
    outString->append((char*)contents, totalSize);
    return totalSize;
}

size_t WriteCallbackPic(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* out = static_cast<std::ofstream*>(userp);
    size_t totalSize = size * nmemb;
    out->write(static_cast<char*>(contents), totalSize);
    return totalSize;
}

bool dlImg(const std::string& url, const std::string& filePath) {
    CURL* curl;
    CURLcode res;

    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) return false;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "./pem/cacert.pem");
        curl_easy_setopt(curl, CURLOPT_CAPATH, "./pem/cacert.pem");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackPic);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        qDebug() << "Arrive here";

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    outFile.close();

    return res == CURLE_OK;
}

bool downloadImg(std::string url) {
    int start = 32, len = url.length();
    PICPATH = "./" + url.substr(start, len - start);
    qDebug() << url;

    // Check if the file exists
    if (!std::filesystem::exists(PICPATH)) {
        // Create the necessary directories
        std::filesystem::create_directories(std::filesystem::path(PICPATH).parent_path());

        qDebug() << "Before get image";
        qDebug() << "The path: " << QString::fromUtf8(PICPATH);
        // Download the image
        bool getImg = dlImg(url, PICPATH);
        if (getImg) return true;
        else return false;
    }

    return true;
}

std::wstring convertSTR(std::string str) {
    // Optionally convert to a wide string (UTF-16 or UTF-32)
    // This is for Windows. Use std::wstring_convert for cross-platform
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wstr = L"";
    try {
        // Convert UTF-8 string to wide string
        wstr = converter.from_bytes(str);
    }
    catch (const std::range_error& e) {
        qDebug() << "Range error: " << e.what();
    }
    return wstr;
}

int makeGet() {
    // Initialize libcurl
    CURL* curl = curl_easy_init();

    if (curl) {
        // Define a string to hold the response data
        std::string responseString;

        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "./pem/cacert.pem");
        curl_easy_setopt(curl, CURLOPT_CAPATH, "./pem/cacert.pem");

        // Set the WriteCallback function to store response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        // Specify the string to pass to the callback function to collect the response
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        // Perform the request and check for errors
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            qDebug() << "curl_easy_perform() failed: " << curl_easy_strerror(res);
            return 1;
        }
        else {
            // Output the response
            // If request was successful, parse the JSON response
            try {
                json jsonResponse = json::parse(responseString);

                if (jsonResponse["data"].size() == 0) return 2;

                TITLE = convertSTR(std::string(jsonResponse["data"][0]["title"]));
                AUTHOR = convertSTR(std::string(jsonResponse["data"][0]["author"]));
                UID = convertSTR(std::to_string(int(jsonResponse["data"][0]["uid"])));

                int taglen = jsonResponse["data"][0]["tags"].size();
                for (int i = 0; i < taglen; i++) {
                    TAGS += (convertSTR(std::string(jsonResponse["data"][0]["tags"][i])) + L"  ");
                    if ((i + 1) % 7 == 0) TAGS += L"\n";
                }

                bool imgStat = downloadImg(std::string(jsonResponse["data"][0]["urls"]["original"]));
                if (!imgStat) return 3;

            } catch (json::parse_error& e) {
                qDebug() << "JSON parse error: " << e.what();
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }

    return 0;
}

int openPicFile() {
    if (HASPIC) {
        // Specify the path to the .jpg file
        std::string nowPath = "";
        int start = 0, len = 0;
        for (int i = 0; i < PICPATH.length(); i++) {
            if (PICPATH[i] == '/') {
                nowPath += (PICPATH.substr(start, len) + "\\");
                start = i + 1;
                len = 0;
            }
            else len++;
        }
        nowPath += (PICPATH.substr(start, len));
        LPCSTR imagePath = nowPath.c_str();

        // Use ShellExecute to open the .jpg file with the default image viewer
        HINSTANCE result = ShellExecuteA(NULL, "open", imagePath, NULL, NULL, SW_SHOWDEFAULT);

        // Check if the operation was successful
        if (reinterpret_cast<intptr_t>(result) <= 32) {
            // Handle the error
            DWORD error = GetLastError();
            // You can use error codes to diagnose the issue
            switch (error) {
            case ERROR_FILE_NOT_FOUND:
                MessageBoxA(NULL, "File not found.", "Error", MB_OK);
                break;
            case ERROR_PATH_NOT_FOUND:
                MessageBoxA(NULL, "Path not found.", "Error", MB_OK);
                break;
            case ERROR_ACCESS_DENIED:
                MessageBoxA(NULL, "Access denied.", "Error", MB_OK);
                break;
            default:
                MessageBoxA(NULL, "Unknown error occurred.", "Error", MB_OK);
                break;
            }
            return -1;
        }

        return 0;
    }
    else return 1;
    return -1;
}

// Slot for QPushButton click
void Setu::handleButtonClicked()
{
    ui->labelHint->clear();
    ui->labelHint->setText("加载中...");
    HASPIC = false;
    ui->labelImage->clear();
    ui->labelImage->resize(700, 370);
    ui->labelTextBelow->clear();
    URL = "https://api.lolicon.app/setu/v2";
    TITLE = L"";
    AUTHOR = L"";
    UID = L"";
    TAGS = L"";
    PICPATH = "";

    // 1. Retrieve the content of QTextEdit
    QString tagContent = ui->textEdit->toPlainText();
    QString uidContent = ui->textEdit_2->toPlainText();

    // 2. Check the status of QCheckBox
    bool isChecked = ui->checkBox->isChecked();
    bool isChecked2 = ui->checkBox_2->isChecked();
    bool isChecked3 = ui->checkBox_3->isChecked();
    bool isChecked4 = ui->checkBox_4->isChecked();

    int op = getRequestURL(tagContent, uidContent, isChecked, isChecked2, isChecked3, isChecked4);

    switch(op) {
    case 0: {
        qDebug() << QString::fromUtf8(URL);

        int opGet = makeGet();

        switch(opGet) {
        case 1: {
            ui->labelHint->clear();
            ui->labelHint->setText("请求失败");
            break;
        }
        case 2: {
            ui->labelHint->clear();
            ui->labelHint->setText("tag输入有误");
            break;
        }
        case 3: {
            ui->labelHint->clear();
            ui->labelHint->setText("404 NOT FOUND!");
            break;
        }
        case 0: {
            ui->labelHint->clear();
            ui->labelHint->setText(QString::fromUtf8(std::string("当前图片路径: setu") + PICPATH));
            HASPIC = true;
            // 3. Display an image in QLabel
            QString imagePath = QString::fromUtf8(PICPATH);
            QPixmap pixmap(imagePath);  // Replace with your image path
            // Get the label's size to scale the image appropriately while maintaining the aspect ratio
            QSize picSize = pixmap.size();
            double picRatio = double(picSize.width()) / double(picSize.height());
            if (picRatio < LABELRATIO) {
                double labelWidth = double(370) * double(picSize.width()) / double(picSize.height());
                ui->labelImage->resize(int(labelWidth), 370);
                ui->labelImage->move(int((double(800) - labelWidth) / 2.0), 90);
            }
            else if (picRatio > LABELRATIO) {
                double labelHeight = double(700) / double(picSize.width()) * double(picSize.height());
                ui->labelImage->resize(700, int(labelHeight));
                ui->labelImage->move(50, 90 + int((double(370) - labelHeight) / 2.0));
            }
            // Display the scaled pixmap in the QLabel
            ui->labelImage->setPixmap(pixmap);
            ui->labelImage->setScaledContents(true); // Optional, scales the image to fit the label size

            // 4. Display the QTextEdit content below the QTextEdit
            std::wstring textShow = L"";
            textShow += (std::wstring(L"title: ") + TITLE + std::wstring(L"\n"));
            textShow += (std::wstring(L"author: ") + AUTHOR + std::wstring(L"\n"));
            textShow += (std::wstring(L"uid: ") + UID + std::wstring(L"\n"));
            textShow += (std::wstring(L"tags: ") + TAGS + std::wstring(L"\n"));
            ui->labelTextBelow->setText(QString::fromWCharArray(textShow.c_str(), static_cast<int>(textShow.length())));

            break;
        }
        }

        break;
    }
    case 1: {
        ui->labelHint->setText("tag输入错误");
        break;
    }
    case 2: {
        ui->labelHint->setText("uid输入错误");
        break;
    }
    case 3: {
        ui->labelHint->setText("图片比例错误");
        break;
    }
    }

}

void Setu::handleClearButtonClicked() {
    ui->labelHint->clear();
    HASPIC = false;
    ui->labelImage->clear();
    ui->labelImage->resize(700, 370);
    ui->labelImage->move(50, 90);
    ui->labelTextBelow->clear();
    URL = "https://api.lolicon.app/setu/v2";
    TITLE = L"";
    AUTHOR = L"";
    UID = L"";
    TAGS = L"";
    PICPATH = "";
}

void Setu::orgPicClicked() {
    qDebug() << "Click called";
    int op = openPicFile();
    if (op < 0) qDebug() << "Open file failed";
    else if (op == 1) {
        ui->labelHint->clear();
        ui->labelHint->setText("没有图片");
    }
}
