local base_dir = path.getabsolute("..")

solution "pecs" do
	configurations { "Debug", "Release" }

	targetdir(path.join(base_dir, "bin"))  -- force our bins into bin/
	location(path.join(base_dir, "build")) -- and the makefiles into build/ so we can make -C build

	configuration { "Release" }
	flags {
		"OptimizeSpeed"
	}

	configuration { "Debug" }
	flags {
		"Symbols"
	}
end

project "pecs" do
	kind "WindowedApp"
	language "C++"
	targetname "pecs"

	configuration {"gmake"}
	buildoptions {
		"-O3",
		"-ffast-math",
		"-std=c++11",
		"-Wall",
	}

	local src_dir  = path.join(base_dir, "example")
	configuration {}
	files {
		path.join(src_dir, "**.cpp"),
		path.join(src_dir, "**.hpp")
	}

	includedirs {
		path.join(base_dir, "include"),
		src_dir,
	}
end
