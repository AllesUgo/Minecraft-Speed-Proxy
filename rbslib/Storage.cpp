#include "Storage.h"
#include "Storage.h"
#include "Storage.h"
#include "FileIO.h"
#ifdef WIN32
#include <io.h>
#endif
#ifdef LINUX
#include <unistd.h>
#endif // linux

void RbsLib::Storage::StorageFile::CheckFileExist(void) const
{
	if (!this->IsExist()) throw RbsLib::Storage::StorageException("No such file or directory", RbsLib::Storage::StorageException::Reason::FileAbsent);
}

void RbsLib::Storage::StorageFile::PermissionException(RbsLib::Storage::FilePermission perms) const
{
	if ((this->Permission() & perms) != perms) throw RbsLib::Storage::StorageException("Insufficient permissions", RbsLib::Storage::StorageException::Reason::FilePermissionError);
}

RbsLib::Storage::StorageFile::StorageFile(void)noexcept
{
	this->path = "";
}

RbsLib::Storage::StorageFile::StorageFile(const std::string& path) noexcept
{
	this->path = path;
}

RbsLib::Storage::StorageFile::StorageFile(const std::filesystem::path& path) noexcept :path(path) {}

RbsLib::Storage::StorageFile::StorageFile(const char* path) :path(path) {}

RbsLib::Storage::FileType::FileType RbsLib::Storage::StorageFile::GetFileType(void) const
{
	this->CheckFileExist();
	this->PermissionException(RbsLib::Storage::FilePermission::Read);
	int a = (int)std::filesystem::status(this->path).type();

	switch (std::filesystem::status(this->path).type())
	{
	case std::filesystem::file_type::regular:
		return RbsLib::Storage::FileType::Regular;
	case std::filesystem::file_type::directory:
		return RbsLib::Storage::FileType::Dir;
	default:
		return RbsLib::Storage::FileType::Other;
	}
}

std::string RbsLib::Storage::StorageFile::Path(void) const noexcept
{
	return this->path.string();
}

bool RbsLib::Storage::StorageFile::IsExist(void) const noexcept
{
	return std::filesystem::exists(this->path);
}
uint64_t RbsLib::Storage::StorageFile::GetFileSize(void) const
{
	if (this->GetFileType() != RbsLib::Storage::FileType::Regular) throw RbsLib::Storage::StorageException("File type error", RbsLib::Storage::StorageException::Reason::FileTypeError);
	this->PermissionException(RbsLib::Storage::FilePermission::Read);
	return std::filesystem::file_size(this->path);
}

RbsLib::Storage::StorageFile& RbsLib::Storage::StorageFile::Append(const std::string& str) noexcept
{
	this->path.append(str);
	return *this;
}

RbsLib::Storage::StorageFile RbsLib::Storage::StorageFile::Parent(void) const
{
	return RbsLib::Storage::StorageFile(this->path.parent_path());
}

RbsLib::Storage::StorageFile RbsLib::Storage::StorageFile::operator[](const std::string& sub_path) const
{
	auto p = *this;
	p.Append(sub_path);
	return p;
}

std::string RbsLib::Storage::StorageFile::GetName(void) const
{
	return this->path.filename().string();
}

int RbsLib::Storage::StorageFile::Remove(void) const
{
	this->CheckFileExist();
	std::filesystem::remove(this->path);
	return 0;
}

int RbsLib::Storage::StorageFile::RemoveAll(void) const
{
	this->CheckFileExist();
	std::filesystem::remove_all(this->path);
	return 0;
}

bool RbsLib::Storage::StorageFile::CreateDir(const std::string& dir_name) const
{
	this->CheckFileExist();
	if (this->GetFileType() != RbsLib::Storage::FileType::Dir)
		throw RbsLib::Storage::StorageException(this->Path() + " not a dir", RbsLib::Storage::StorageException::Reason::FileTypeError);
	return std::filesystem::create_directory((*this)[dir_name].path);
}

bool RbsLib::Storage::StorageFile::CreateDirs(const std::string& dir_path) const
{
	this->CheckFileExist();
	if (this->GetFileType() != RbsLib::Storage::FileType::Dir)
		throw RbsLib::Storage::StorageException(this->Path() + " not a dir", RbsLib::Storage::StorageException::Reason::FileTypeError);
	return std::filesystem::create_directories((*this)[dir_path].path);
}

std::string RbsLib::Storage::StorageFile::GetStem(void) const
{
	return std::filesystem::path(this->Path()).stem().string();
}

std::string RbsLib::Storage::StorageFile::GetExtension(void) const
{
	return std::filesystem::path(this->Path()).extension().string();
}

RbsLib::Storage::FilePermission RbsLib::Storage::StorageFile::Permission(void) const noexcept
{
	this->CheckFileExist();
	RbsLib::Storage::FilePermission permissions = RbsLib::Storage::FilePermission::None;
	auto perm = std::filesystem::status(this->path).permissions();
	namespace fs = std::filesystem;
	namespace ts = RbsLib::Storage;
	if ((perm & fs::perms::group_exec) == fs::perms::group_exec) permissions |= FilePermission::GroupExecute;
	if ((perm & fs::perms::group_read) == fs::perms::group_read) permissions |= FilePermission::GroupRead;
	if ((perm & fs::perms::group_write) == fs::perms::group_write) permissions |= FilePermission::GroupWrite;
	if ((perm & fs::perms::owner_exec) == fs::perms::owner_exec) permissions |= FilePermission::OwnerExecute;
	if ((perm & fs::perms::owner_read) == fs::perms::owner_read) permissions |= FilePermission::OwnerRead;
	if ((perm & fs::perms::owner_write) == fs::perms::owner_write) permissions |= FilePermission::OwnerWrite;
	if ((perm & fs::perms::others_exec) == fs::perms::others_exec) permissions |= FilePermission::OtherExecute;
	if ((perm & fs::perms::others_read) == fs::perms::others_read) permissions |= FilePermission::OtherRead;
	if ((perm & fs::perms::others_write) == fs::perms::others_write) permissions |= FilePermission::OtherWrite;
	if (!access(this->path.string().c_str(), 2)) permissions |= FilePermission::Read;
	if (!access(this->path.string().c_str(), 4)) permissions |= FilePermission::Write;
	if (!access(this->path.string().c_str(), 6)) permissions |= FilePermission::Execute;
	return permissions;
}

RbsLib::Storage::StorageFile::Iterator RbsLib::Storage::StorageFile::begin() const
{
	if (this->GetFileType() != RbsLib::Storage::FileType::Dir) throw RbsLib::Storage::StorageException("File type error", RbsLib::Storage::StorageException::Reason::FileTypeError);
	this->PermissionException(RbsLib::Storage::FilePermission::Read);
	return std::filesystem::begin(std::filesystem::directory_iterator(this->path));
}

RbsLib::Storage::StorageFile::Iterator RbsLib::Storage::StorageFile::end() const
{
	return std::filesystem::end(std::filesystem::directory_iterator(this->path));
}

RbsLib::Storage::FileIO::File RbsLib::Storage::StorageFile::Open(RbsLib::Storage::FileIO::OpenMode open_mode, RbsLib::Storage::FileIO::SeekBase default_seek, int64_t default_offset) const
{
	return RbsLib::Storage::FileIO::File(this->Path(), open_mode, default_seek, default_offset);
}

RbsLib::Storage::StorageFile::Iterator::Iterator(const std::filesystem::directory_iterator& iterator) noexcept :iterator(iterator) {}

RbsLib::Storage::FilePermission RbsLib::Storage::operator|(FilePermission left, FilePermission right) noexcept
{
	return (FilePermission)((int)left | (int)right);
}

RbsLib::Storage::FilePermission RbsLib::Storage::operator|=(FilePermission& left, FilePermission right) noexcept
{
	left = (FilePermission)((int)left | (int)right);
	return left;
}

RbsLib::Storage::FilePermission RbsLib::Storage::operator&(FilePermission left, FilePermission right) noexcept
{
	return RbsLib::Storage::FilePermission((int)left & (int)right);
}

RbsLib::Storage::FilePermission RbsLib::Storage::operator&=(FilePermission& left, FilePermission right) noexcept
{
	left = RbsLib::Storage::FilePermission((int)left & (int)right);
	return left;
}
