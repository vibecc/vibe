#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {

        web.guard(5); // 5 seconds cool down

        web.send("Hello World");
    }});


    router.listen();
}