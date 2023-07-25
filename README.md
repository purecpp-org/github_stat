# github_stat

```
git clone --recurse-submodules -j20 https://github.com/purecpp-org/github_stat.git

cd github_stat
makedir build
cd build
cmake ..
make -j

./github_stat your_github_token your_reponame

// your_reponame such as qicosmos/cinatra
```