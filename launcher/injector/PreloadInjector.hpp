#pragma once

class QString;

namespace launcher::injector {
class PreloadInjector {
public:
    bool launch(const QString &launchAppArguments, const QString &probeDllPath);
private:
};
}
