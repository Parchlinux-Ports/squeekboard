#[macro_use]
extern crate clap;
extern crate rs;

use rs::tests::check_layout_file;

fn main() -> () {
    #[cfg(feature = "clap_v4")]
    let matches = clap::Command::new("squeekboard-test-layout")
        .about("Test keyboard layout for errors. Returns OK or an error message containing further information.")
        .arg(
            clap::Arg::new("INPUT")
                .required(true)
                .help("Yaml keyboard layout file to test")
        )
        .get_matches();
    #[cfg(feature = "clap_v4")]
    let m = matches.get_one::<String>("INPUT");

    #[cfg(not(feature = "clap_v4"))]
    let matches = clap_app!(test_layout =>
        (name: "squeekboard-test-layout")
        (about: "Test keyboard layout for errors. Returns OK or an error message containing further information.")
        (@arg INPUT: +required "Yaml keyboard layout file to test")
    ).get_matches();
    #[cfg(not(feature = "clap_v4"))]
    let m = matches.value_of("INPUT");

    if check_layout_file(m.unwrap()) == () {
        println!("Test result: OK");
    }
}
