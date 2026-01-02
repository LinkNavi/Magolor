class PackageEntry {
	pub name: string;
	pub votes: int;
	pub fn addUpvote() {
		
	}
}

pub fn createPackage(name: string) -> PackageEntry {
      let p = new PackageEntry();

      p.name = name;
      p.votes = 0;

      return p;
}
