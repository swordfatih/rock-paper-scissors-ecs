add_repositories("xrepo_fatih https://github.com/swordfatih/xmake-repo.git main")
add_requires("sfml-nocmake 2.6.0", "cntity view") 

target("rps")
    set_languages("c++20")
    add_packages("sfml-nocmake", "cntity") 
    add_rules("mode.release")
	add_files("src/main.cpp")
    set_rundir("$(projectdir)/runtime")

    after_build(function (target)
        os.cp(target:targetfile(), "$(scriptdir)/runtime")
    end)