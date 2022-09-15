/*! Tests if any layout files are not in use */

extern crate rs;

use rs::resources;
use std::env;
use std::error::Error;
use std::ffi::OsStr;
use std::fs;
use std::path::{Path, PathBuf};

enum Orphans {
    None,
    Present,
}

fn check(base: &Path, dir: &Path) -> Result<Orphans, Box<dyn Error>> {
    let mut orphans = Orphans::None;
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let path = entry.path();

        if entry.file_type()?.is_dir() {
            check(base, &path)?;
        } else {
            if Some(OsStr::new("yaml")) == path.extension() {
                let resource_path = path
                    .strip_prefix(base).unwrap()
                    .with_extension("");
                let resource_path = resource_path
                    .to_str().unwrap();
                let resource_path = resource_path
                    .strip_prefix('/').unwrap_or(resource_path);
                if let None = resources::get_keyboard(resource_path) {
                    println!("Data not registered in the resources file: {:?}", path);
                    orphans = Orphans::Present;
                }
            }
        }
    }
    Ok(orphans)
}

fn main() -> () {
    let path = env::args().nth(1).expect("Provide a path");
    let path = PathBuf::from(path);

    match check(&path, &path) {
        Err(e) => panic!("{:?}", e),
        Ok(Orphans::Present) => panic!("Unregistered files present. Check the tutorial in doc/tutorial.md"),
        _ => {},
    }
}
